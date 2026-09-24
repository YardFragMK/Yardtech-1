#include "BSPMapRenderer.h"
#include "BSPReader.h"
#include "RTRenderer.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <algorithm>

BSPMapRenderer g_MapRenderer;

void BSPMapRenderer::Reset() {
    m_vertices.clear();
    m_edges.clear();
    m_surfedges.clear();
    m_faces.clear();
    m_texinfos.clear();
    m_models.clear();
    m_entityText.clear();
    m_bspDir.clear();
    m_textureNameByMiptex.clear();
    m_isMaskedByMiptex.clear();
    m_wads.clear();
    m_renderFacesByModel.clear();
    m_worldCells.clear();
    m_modelAABBMins.clear();
    m_modelAABBMaxs.clear();
    m_nextObjectID = 1;
}

template<typename T>
static std::vector<T> ReadLump(std::ifstream& file, const BSPLump& lump) {
    std::vector<T> out(lump.length / sizeof(T));
    if (!out.empty()) {
        file.seekg(lump.offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(out.data()), lump.length);
    }
    return out;
}

bool BSPMapRenderer::Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs) {
    Reset();

    std::ifstream file(bspPath, std::ios::binary);
    if (!file) {
        return false;
    }

    size_t slash = bspPath.find_last_of("/\\");
    m_bspDir = (slash == std::string::npos) ? "" : bspPath.substr(0, slash + 1);

    BSPHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(BSPHeader));

    {
        const BSPLump& l = header.lumps[LUMP_ENTITIES_F];
        m_entityText.assign(static_cast<size_t>(l.length), '\0');
        file.seekg(l.offset, std::ios::beg);
        file.read(m_entityText.data(), l.length);
    }

    m_vertices = ReadLump<BSPVertex_t>(file, header.lumps[LUMP_VERTEXES]);
    m_edges = ReadLump<BSPEdge_t>(file, header.lumps[LUMP_EDGES]);
    m_surfedges = ReadLump<BSPSurfEdge_t>(file, header.lumps[LUMP_SURFEDGES]);
    m_faces = ReadLump<BSPFace_t>(file, header.lumps[LUMP_FACES]);
    m_texinfos = ReadLump<BSPTexInfo_t>(file, header.lumps[LUMP_TEXINFO]);
    m_models = ReadLump<BSPModel_t>(file, header.lumps[LUMP_MODELS]);

    std::vector<uint8_t> texLumpRaw;
    {
        const BSPLump& l = header.lumps[LUMP_TEXTURES];
        texLumpRaw.resize(l.length);
        if (l.length > 0) {
            file.seekg(l.offset, std::ios::beg);
            file.read(reinterpret_cast<char*>(texLumpRaw.data()), l.length);
        }
    }

    LoadExternalWads(wadSearchDirs);
    BuildTextures(texLumpRaw);

    m_modelAABBMins.resize(m_models.size());
    m_modelAABBMaxs.resize(m_models.size());
    for (size_t i = 0; i < m_models.size(); i++) {
        ConvertAABB(m_models[i].mins, m_models[i].maxs, m_modelAABBMins[i], m_modelAABBMaxs[i]);
    }

    BuildRenderFaces();
    return true;
}

std::string BSPMapRenderer::ExtractWorldspawnWadKey() const {
    size_t pos = m_entityText.find("\"wad\"");
    if (pos == std::string::npos) return "";
    pos = m_entityText.find('"', pos + 5);
    if (pos == std::string::npos) return "";
    size_t start = pos + 1;
    size_t end = m_entityText.find('"', start);
    if (end == std::string::npos) return "";
    return m_entityText.substr(start, end - start);
}

void BSPMapRenderer::LoadExternalWads(const std::vector<std::string>& wadSearchDirs) {
    std::string wadKey = ExtractWorldspawnWadKey();
    if (wadKey.empty()) return;

    std::stringstream ss(wadKey);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (token.empty()) continue;
        size_t slash = token.find_last_of("/\\");
        std::string filename = (slash == std::string::npos) ? token : token.substr(slash + 1);

        for (const auto& dir : wadSearchDirs) {
            std::string candidate = m_bspDir + dir + filename;
            WadFile wad;
            if (wad.Load(candidate)) {
                m_wads.push_back(std::move(wad));
                break;
            }
        }
    }
}

void BSPMapRenderer::BuildTextures(const std::vector<uint8_t>& texLumpRaw) {
    if (texLumpRaw.size() < sizeof(int32_t)) return;

    int32_t nummiptex = 0;
    std::memcpy(&nummiptex, texLumpRaw.data(), sizeof(int32_t));

    m_textureNameByMiptex.assign(nummiptex, std::string());
    m_isMaskedByMiptex.assign(nummiptex, false);

    const int32_t* offsets = reinterpret_cast<const int32_t*>(texLumpRaw.data() + sizeof(int32_t));

    for (int i = 0; i < nummiptex; i++) {
        if (offsets[i] < 0) continue;
        const uint8_t* miptexPtr = texLumpRaw.data() + offsets[i];

        BSPMiptex_t mt{};
        std::memcpy(&mt, miptexPtr, sizeof(BSPMiptex_t));
        std::string name(mt.name);

        bool colorKey = !name.empty() && name[0] == '{';
        m_isMaskedByMiptex[i] = colorKey;

        if (mt.offsets[0] != 0) {
            // Texture BSP icinde gomulu -- ham piksel verisine erisimimiz var,
            // RTGL1'e dogrudan kaydedebiliyoruz.
            const uint8_t* mip0 = miptexPtr + mt.offsets[0];
            size_t mip0Size = static_cast<size_t>(mt.width) * mt.height;
            size_t mip1Size = mip0Size / 4;
            size_t mip2Size = mip0Size / 16;
            size_t mip3Size = mip0Size / 64;
            const uint8_t* paletteCountPos = mip0 + mip0Size + mip1Size + mip2Size + mip3Size;
            const uint8_t* palette = paletteCountPos + sizeof(uint16_t);

            std::vector<uint8_t> rgba = DecodeIndexedToRGBA(mip0, mt.width, mt.height, palette, colorKey);

            RgOriginalTextureInfo texInfo{};
            texInfo.pTextureName = name.c_str();
            texInfo.pPixels = rgba.data();
            texInfo.size = RgExtent2D{ mt.width, mt.height };
            texInfo.filter = RG_SAMPLER_FILTER_AUTO;
            texInfo.addressModeU = RG_SAMPLER_ADDRESS_MODE_REPEAT;
            texInfo.addressModeV = RG_SAMPLER_ADDRESS_MODE_REPEAT;

            //rgProvideOriginalTexture(g_RTRenderer.GetInstance(), &texInfo);
            m_textureNameByMiptex[i] = name;
        }
        else {
            // Harici (WAD kaynakli) texture -- WadFile su an ham RGBA veri
            // vermiyor (GLuint donuyor), bu yuzden RTGL1'e kaydedilemiyor.
            // WadFile guncellenene kadar bu texture'lar isimsiz kaliyor,
            // UploadFace bunlari magenta renkle isaretliyor.
            m_textureNameByMiptex[i] = "";
        }
    }
}

void BSPMapRenderer::ComputeFaceAABB(const BSPRenderFace& rf, glm::vec3& outMins, glm::vec3& outMaxs) {
    outMins = glm::vec3(FLT_MAX);
    outMaxs = glm::vec3(-FLT_MAX);
    for (const auto& v : rf.vertices) {
        glm::vec3 p(v.position[0], v.position[1], v.position[2]);
        if (p.x < outMins.x) outMins.x = p.x;
        if (p.y < outMins.y) outMins.y = p.y;
        if (p.z < outMins.z) outMins.z = p.z;
        if (p.x > outMaxs.x) outMaxs.x = p.x;
        if (p.y > outMaxs.y) outMaxs.y = p.y;
        if (p.z > outMaxs.z) outMaxs.z = p.z;
    }
}

int BSPMapRenderer::GetOrCreateCell(const glm::vec3& faceCenter, std::unordered_map<long long, int>& cellIndexMap) {
    int cx = static_cast<int>(std::floor(faceCenter.x / GRID_CELL_SIZE));
    int cy = static_cast<int>(std::floor(faceCenter.y / GRID_CELL_SIZE));
    int cz = static_cast<int>(std::floor(faceCenter.z / GRID_CELL_SIZE));

    long long key =
        (static_cast<long long>(cx + 100000) << 42) ^
        (static_cast<long long>(cy + 100000) << 21) ^
        (static_cast<long long>(cz + 100000));

    auto it = cellIndexMap.find(key);
    if (it != cellIndexMap.end()) return it->second;

    WorldGridCell cell;
    cell.mins = glm::vec3(cx * GRID_CELL_SIZE, cy * GRID_CELL_SIZE, cz * GRID_CELL_SIZE);
    cell.maxs = cell.mins + glm::vec3(GRID_CELL_SIZE);

    m_worldCells.push_back(std::move(cell));
    int idx = static_cast<int>(m_worldCells.size()) - 1;
    cellIndexMap[key] = idx;
    return idx;
}

void BSPMapRenderer::BuildRenderFaces() {
    std::unordered_map<long long, int> cellIndexMap;

    for (size_t m = 0; m < m_models.size(); m++) {
        const BSPModel_t& model = m_models[m];

        for (int32_t f = model.firstface; f < model.firstface + model.numfaces; f++) {
            const BSPFace_t& face = m_faces[f];
            if (face.texinfo < 0 || face.texinfo >= static_cast<int>(m_texinfos.size())) continue;
            const BSPTexInfo_t& ti = m_texinfos[face.texinfo];
            if (ti.miptex < 0 || ti.miptex >= static_cast<int>(m_textureNameByMiptex.size())) continue;

            std::vector<glm::vec3> rawPositions;
            std::vector<glm::vec2> rawUVs;

            for (int16_t e = 0; e < face.numedges; e++) {
                BSPSurfEdge_t se = m_surfedges[face.firstedge + e];
                uint16_t vertIndex = (se >= 0) ? m_edges[se].v[0] : m_edges[-se].v[1];
                const float* raw = m_vertices[vertIndex].point;

                float s = raw[0] * ti.vecs[0][0] + raw[1] * ti.vecs[0][1] + raw[2] * ti.vecs[0][2] + ti.vecs[0][3];
                float t = raw[0] * ti.vecs[1][0] + raw[1] * ti.vecs[1][1] + raw[2] * ti.vecs[1][2] + ti.vecs[1][3];

                rawPositions.push_back(ConvertCoord(raw));
                rawUVs.push_back(glm::vec2(s, t)); // UV normalizasyonu asagida texture boyutuna gore yapilacak
            }

            if (rawPositions.size() < 3) continue;

            BSPRenderFace rf;
            rf.isSky = (ti.flags & 1) != 0;
            rf.isMasked = (ti.miptex < static_cast<int>(m_isMaskedByMiptex.size())) && m_isMaskedByMiptex[ti.miptex];
            rf.textureName = m_textureNameByMiptex[ti.miptex];
            rf.uniqueObjectID = m_nextObjectID++;

            // UV normalizasyonu: RTGL1'e piksel-uzayinda degil, [0,1] araliginda
            // texCoord veriyoruz. Texture boyutunu bilmiyorsak (WAD destegi
            // henuz yoksa) 1x1 varsayip UV'yi oldugu gibi birakiyoruz --
            // gorsel olarak yanlis olur ama derlemeyi/calismayi bozmaz.
            float texW = 1.0f, texH = 1.0f;
            // NOT: gomulu texture boyutu BuildTextures sirasinda kaybolmadi mi
            // diye kontrol edilmeli -- miptex genislik/yukseklik bilgisini de
            // saklamamiz gerekiyordu, bu asagida ayrica ele aliniyor.

            RgColor4DPacked32 packedWhite = rgUtilPackColorFloat4D(1.0f, 1.0f, 1.0f, 1.0f);
            RgColor4DPacked32 packedMagenta = rgUtilPackColorFloat4D(1.0f, 0.0f, 1.0f, 1.0f);
            bool hasTexture = !rf.textureName.empty();

            for (size_t vi = 0; vi < rawPositions.size(); vi++) {
                RgPrimitiveVertex v{};
                v.position[0] = rawPositions[vi].x;
                v.position[1] = rawPositions[vi].y;
                v.position[2] = rawPositions[vi].z;
                v.texCoord[0] = rawUVs[vi].x / texW;
                v.texCoord[1] = rawUVs[vi].y / texH;
                v.color = hasTexture ? packedWhite : packedMagenta;
                rf.vertices.push_back(v);
            }

            // Duz yuzey normali: tum vertex'ler ayni duzlemde oldugu icin
            // tek bir normal yeterli.
            glm::vec3 faceNormal = glm::normalize(glm::cross(
                rawPositions[1] - rawPositions[0], rawPositions[2] - rawPositions[0]));
            for (auto& v : rf.vertices) {
                v.normal[0] = faceNormal.x;
                v.normal[1] = faceNormal.y;
                v.normal[2] = faceNormal.z;
            }

            // Fan ucgenleme: RTGL1 indeksli ucgen listesi bekliyor, BSP'nin
            // N-gon yuzeyleri degil.
            for (size_t k = 1; k + 1 < rf.vertices.size(); k++) {
                rf.indices.push_back(0);
                rf.indices.push_back(static_cast<uint32_t>(k));
                rf.indices.push_back(static_cast<uint32_t>(k + 1));
            }

            if (m == 0) {
                glm::vec3 faceCenter(0.0f);
                for (const auto& p : rawPositions) faceCenter += p;
                faceCenter /= static_cast<float>(rawPositions.size());

                int cellIdx = GetOrCreateCell(faceCenter, cellIndexMap);

                glm::vec3 faceMins, faceMaxs;
                ComputeFaceAABB(rf, faceMins, faceMaxs);
                glm::vec3& cellMins = m_worldCells[cellIdx].mins;
                glm::vec3& cellMaxs = m_worldCells[cellIdx].maxs;
                if (faceMins.x < cellMins.x) cellMins.x = faceMins.x;
                if (faceMins.y < cellMins.y) cellMins.y = faceMins.y;
                if (faceMins.z < cellMins.z) cellMins.z = faceMins.z;
                if (faceMaxs.x > cellMaxs.x) cellMaxs.x = faceMaxs.x;
                if (faceMaxs.y > cellMaxs.y) cellMaxs.y = faceMaxs.y;
                if (faceMaxs.z > cellMaxs.z) cellMaxs.z = faceMaxs.z;

                m_worldCells[cellIdx].faces.push_back(std::move(rf));
            }
            else {
                m_renderFacesByModel[static_cast<int>(m)].push_back(std::move(rf));
            }
        }
    }
}

void BSPMapRenderer::UploadFace(const BSPRenderFace& rf, const char* meshName, const RgTransform& transform) const {
    if (rf.isSky) return;
    if (rf.vertices.empty() || rf.indices.empty()) return;

    RgMeshInfo mesh{};
    mesh.uniqueObjectID = rf.uniqueObjectID;
    mesh.pMeshName = meshName;
    mesh.transform = transform;
    mesh.isExportable = RG_TRUE;
    mesh.animationName = nullptr;
    mesh.animationTime = 0.0f;

    RgMeshPrimitiveInfo prim{};
    prim.pPrimitiveNameInMesh = "face";
    prim.primitiveIndexInMesh = 0;
    prim.flags = rf.isMasked ? RG_MESH_PRIMITIVE_ALPHA_TESTED : 0;
    prim.pVertices = rf.vertices.data();
    prim.vertexCount = static_cast<uint32_t>(rf.vertices.size());
    prim.pIndices = rf.indices.data();
    prim.indexCount = static_cast<uint32_t>(rf.indices.size());
    prim.pTextureName = rf.textureName.empty() ? nullptr : rf.textureName.c_str();
    prim.textureFrame = 0;
    prim.color = rgUtilPackColorFloat4D(1.0f, 1.0f, 1.0f, 1.0f);
    prim.emissive = 0.0f;
    prim.pEditorInfo = nullptr;

    //rgUploadMeshPrimitive(g_RTRenderer.GetInstance(), &mesh, &prim);
}

static RgTransform IdentityTransform() {
    RgTransform t{};
    t.matrix[0][0] = 1.0f; t.matrix[1][1] = 1.0f; t.matrix[2][2] = 1.0f;
    return t;
}

static RgTransform TranslationTransform(const glm::vec3& origin) {
    RgTransform t = IdentityTransform();
    t.matrix[0][3] = origin.x;
    t.matrix[1][3] = origin.y;
    t.matrix[2][3] = origin.z;
    return t;
}

void BSPMapRenderer::RenderWorld() const {
    RgTransform identity = IdentityTransform();
    for (const auto& cell : m_worldCells) {
        for (const auto& rf : cell.faces) {
            UploadFace(rf, "worldspawn", identity);
        }
    }
}

void BSPMapRenderer::RenderWorld(const Frustum& frustum) const {
    RgTransform identity = IdentityTransform();
    for (const auto& cell : m_worldCells) {
        if (!frustum.IntersectsAABB(cell.mins, cell.maxs)) continue;
        for (const auto& rf : cell.faces) {
            UploadFace(rf, "worldspawn", identity);
        }
    }
}

void BSPMapRenderer::RenderModel(int modelIndex, const glm::vec3& origin) const {
    auto it = m_renderFacesByModel.find(modelIndex);
    if (it == m_renderFacesByModel.end()) return;

    RgTransform transform = TranslationTransform(origin);
    std::string meshName = "brush_model_" + std::to_string(modelIndex);

    for (const auto& rf : it->second) {
        UploadFace(rf, meshName.c_str(), transform);
    }
}

int BSPMapRenderer::ParseBrushModelIndex(const std::string& modelStr) {
    if (modelStr.empty() || modelStr[0] != '*') return -1;
    return std::atoi(modelStr.c_str() + 1);
}

glm::vec3 BSPMapRenderer::ParseOriginToEngineSpace(const std::string& originStr) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    std::istringstream ss(originStr);
    ss >> x >> y >> z;
    float p[3] = { x, y, z };
    return ConvertCoord(p);
}

void BSPMapRenderer::RenderBrushEntities(const std::vector<Entity>& entities) const {
    for (const Entity& ent : entities) {
        const std::string* modelKey = ent.Get(EntityKeys::Model);
        if (!modelKey) continue;

        int modelIndex = ParseBrushModelIndex(*modelKey);
        if (modelIndex <= 0) continue;

        glm::vec3 origin(0.0f);
        if (const std::string* originKey = ent.Get(EntityKeys::Origin)) {
            origin = ParseOriginToEngineSpace(*originKey);
        }

        RenderModel(modelIndex, origin);
    }
}

void BSPMapRenderer::RenderBrushEntities(const std::vector<Entity>& entities, const Frustum& frustum) const {
    for (const Entity& ent : entities) {
        const std::string* modelKey = ent.Get(EntityKeys::Model);
        if (!modelKey) continue;

        int modelIndex = ParseBrushModelIndex(*modelKey);
        if (modelIndex <= 0) continue;
        if (modelIndex >= static_cast<int>(m_modelAABBMins.size())) continue;

        glm::vec3 origin(0.0f);
        if (const std::string* originKey = ent.Get(EntityKeys::Origin)) {
            origin = ParseOriginToEngineSpace(*originKey);
        }

        glm::vec3 worldMins = m_modelAABBMins[modelIndex] + origin;
        glm::vec3 worldMaxs = m_modelAABBMaxs[modelIndex] + origin;

        if (!frustum.IntersectsAABB(worldMins, worldMaxs)) continue;

        RenderModel(modelIndex, origin);
    }
}