#include "BSPMap.h"
#include "BSPReader.h" // BSPHeader / BSPLump (mevcut dosyandaki)
#include <fstream>
#include <sstream>
#include <cstring>
#include "console/Console.h"

template<typename T>
static std::vector<T> ReadLump(std::ifstream& file, const BSPLump& lump) {
    std::vector<T> out(lump.length / sizeof(T));
    if (!out.empty()) {
        file.seekg(lump.offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(out.data()), lump.length);
    }
    return out;
}

bool BSPMap::Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs) {
    std::ifstream file(bspPath, std::ios::binary);
    if (!file) {
        Console::Log("WARNING-> bsp acilamadi: " + bspPath);
        return false;
    }

    size_t slash = bspPath.find_last_of("/\\");
    m_bspDir = (slash == std::string::npos) ? "" : bspPath.substr(0, slash + 1);

    BSPHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(BSPHeader));
    if (header.version != 30) {
        Console::Log("WARNING-> bsp versiyonu uyumlu degil (" + std::to_string(header.version) + ")");
    }

    // --- entity text ---
    {
        const BSPLump& l = header.lumps[LUMP_ENTITIES_F];
        m_entityText.assign(static_cast<size_t>(l.length), '\0');
        file.seekg(l.offset, std::ios::beg);
        file.read(m_entityText.data(), l.length);
    }

    m_entities = ParseEntities(m_entityText);
    m_vertices = ReadLump<BSPVertex_t>(file, header.lumps[LUMP_VERTEXES]);
    m_edges = ReadLump<BSPEdge_t>(file, header.lumps[LUMP_EDGES]);
    m_surfedges = ReadLump<BSPSurfEdge_t>(file, header.lumps[LUMP_SURFEDGES]);
    m_faces = ReadLump<BSPFace_t>(file, header.lumps[LUMP_FACES]);
    m_texinfos = ReadLump<BSPTexInfo_t>(file, header.lumps[LUMP_TEXINFO]);
    m_models = ReadLump<BSPModel_t>(file, header.lumps[LUMP_MODELS]);

    // --- texture lump raw (icinde degisken boyutlu miptex'ler var, struct dizisi degil) ---
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
    BuildRenderFaces();

    Console::Log(bspPath + " yuklendi: " + std::to_string(m_models.size()) + " model, " +
        std::to_string(m_faces.size()) + " face");
    return true;
}

std::string BSPMap::ExtractWorldspawnWadKey() const {
    // basit entity text parse: ilk "{ ... }" blogu worldspawn'dir,
    // icinde "wad" "path1;path2;..." satirini ariyoruz
    size_t pos = m_entityText.find("\"wad\"");
    if (pos == std::string::npos) return "";
    pos = m_entityText.find('"', pos + 5);
    if (pos == std::string::npos) return "";
    size_t start = pos + 1;
    size_t end = m_entityText.find('"', start);
    if (end == std::string::npos) return "";
    return m_entityText.substr(start, end - start);
}

void BSPMap::LoadExternalWads(const std::vector<std::string>& wadSearchDirs) {
    std::string wadKey = ExtractWorldspawnWadKey();
    if (wadKey.empty()) return;

    std::stringstream ss(wadKey);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (token.empty()) continue;
        size_t slash = token.find_last_of("/\\");
        std::string filename = (slash == std::string::npos) ? token : token.substr(slash + 1);

        bool loaded = false;
        for (const auto& dir : wadSearchDirs) {
            std::string candidate = m_bspDir + dir + filename;
            WadFile wad;
            if (wad.Load(candidate)) {
                m_wads.push_back(std::move(wad));
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            Console::Log("WARNING-> harici wad bulunamadi: " + filename);
        }
    }
}

void BSPMap::BuildTextures(const std::vector<uint8_t>& texLumpRaw) {
    if (texLumpRaw.size() < sizeof(int32_t)) return;

    int32_t nummiptex = 0;
    std::memcpy(&nummiptex, texLumpRaw.data(), sizeof(int32_t));

    m_textureIdByMiptex.assign(nummiptex, 0);
    m_textureSizeByMiptex.assign(nummiptex, glm::ivec2(1, 1));

    const int32_t* offsets = reinterpret_cast<const int32_t*>(texLumpRaw.data() + sizeof(int32_t));

    for (int i = 0; i < nummiptex; i++) {
        if (offsets[i] < 0) continue;
        const uint8_t* miptexPtr = texLumpRaw.data() + offsets[i];

        BSPMiptex_t mt{};
        std::memcpy(&mt, miptexPtr, sizeof(BSPMiptex_t));
        std::string name(mt.name);

        m_textureSizeByMiptex[i] = glm::ivec2(mt.width, mt.height);

        if (mt.offsets[0] != 0) {
            // texture bsp icinde gomulu
            const uint8_t* mip0 = miptexPtr + mt.offsets[0];
            size_t mip0Size = static_cast<size_t>(mt.width) * mt.height;
            size_t mip1Size = mip0Size / 4;
            size_t mip2Size = mip0Size / 16;
            size_t mip3Size = mip0Size / 64;
            const uint8_t* paletteCountPos = mip0 + mip0Size + mip1Size + mip2Size + mip3Size;
            const uint8_t* palette = paletteCountPos + sizeof(uint16_t);

            bool colorKey = !name.empty() && name[0] == '{';
            auto rgba = DecodeIndexedToRGBA(mip0, mt.width, mt.height, palette, colorKey);
            m_textureIdByMiptex[i] = CreateGLTextureFromRGBA(rgba, mt.width, mt.height);
        }
        else {
            // gomulu degil, harici wad'larda ara
            GLuint found = 0;
            for (const auto& wad : m_wads) {
                found = wad.GetTexture(name);
                if (found != 0) break;
            }
            if (found == 0) {
                Console::Log("WARNING-> texture bulunamadi: " + name);
            }
            m_textureIdByMiptex[i] = found;
        }
    }
}

void BSPMap::BuildRenderFaces() {
    for (size_t m = 0; m < m_models.size(); m++) {
        const BSPModel_t& model = m_models[m];
        auto& outFaces = m_renderFacesByModel[static_cast<int>(m)];

        for (int32_t f = model.firstface; f < model.firstface + model.numfaces; f++) {
            const BSPFace_t& face = m_faces[f];
            if (face.texinfo < 0 || face.texinfo >= static_cast<int>(m_texinfos.size())) continue;
            const BSPTexInfo_t& ti = m_texinfos[face.texinfo];
            if (ti.miptex < 0 || ti.miptex >= static_cast<int>(m_textureIdByMiptex.size())) continue;

            GLuint glTex = m_textureIdByMiptex[ti.miptex];
            glm::ivec2 texSize = m_textureSizeByMiptex[ti.miptex];

            BSPRenderFace rf;
            rf.glTexture = glTex;

            for (int16_t e = 0; e < face.numedges; e++) {
                BSPSurfEdge_t se = m_surfedges[face.firstedge + e];
                uint16_t vertIndex = (se >= 0) ? m_edges[se].v[0] : m_edges[-se].v[1];
                const float* raw = m_vertices[vertIndex].point;

                float s = raw[0] * ti.vecs[0][0] + raw[1] * ti.vecs[0][1] + raw[2] * ti.vecs[0][2] + ti.vecs[0][3];
                float t = raw[0] * ti.vecs[1][0] + raw[1] * ti.vecs[1][1] + raw[2] * ti.vecs[1][2] + ti.vecs[1][3];

                rf.positions.push_back(ConvertCoord(raw));
                rf.texcoords.push_back(glm::vec2(s / texSize.x, t / texSize.y));
            }

            outFaces.push_back(std::move(rf));
        }
    }
}

static void DrawRenderFace(const BSPRenderFace& rf) {
    if (rf.positions.size() < 3) return;

    if (rf.glTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, rf.glTexture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(1.0f, 0.0f, 1.0f, 1.0f); // texture yoksa magenta (missing texture belirtici)
    }

    glBegin(GL_POLYGON);
    for (size_t i = 0; i < rf.positions.size(); i++) {
        glTexCoord2f(rf.texcoords[i].x, rf.texcoords[i].y);
        glVertex3f(rf.positions[i].x, rf.positions[i].y, rf.positions[i].z);
    }
    glEnd();

    if (rf.glTexture == 0) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // rengi resetle
    }
}

void BSPMap::RenderModel(int modelIndex) const {
    auto it = m_renderFacesByModel.find(modelIndex);
    if (it == m_renderFacesByModel.end()) return;

    glEnable(GL_TEXTURE_2D);
    for (const auto& rf : it->second) {
        DrawRenderFace(rf);
    }
    glDisable(GL_TEXTURE_2D);
}

void BSPMap::RenderWorld() const {
    RenderModel(0); // model 0 = worldspawn (static level geometry)
}

int BSPMap::ParseBrushModelIndex(const std::string& modelStr) {
    if (modelStr.empty() || modelStr[0] != '*') return -1;
    return std::atoi(modelStr.c_str() + 1);
}

glm::vec3 BSPMap::ParseOriginToEngineSpace(const std::string& originStr) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    std::istringstream ss(originStr);
    ss >> x >> y >> z;
    float p[3] = { x, y, z };
    return ConvertCoord(p);
}

void BSPMap::RenderBrushEntities() const {
    for (const Entity& ent : m_entities) {
        const std::string* modelKey = ent.Get(EntityKeys::Model);
        if (!modelKey) continue;

        int modelIndex = ParseBrushModelIndex(*modelKey);
        if (modelIndex <= 0) continue; // 0 = worldspawn, RenderWorld zaten cizer; -1 = brush degil (.mdl vb.)

        glm::vec3 origin(0.0f);
        if (const std::string* originKey = ent.Get(EntityKeys::Origin)) {
            origin = ParseOriginToEngineSpace(*originKey);
        }

        glPushMatrix();
        glTranslatef(origin.x, origin.y, origin.z);
        RenderModel(modelIndex);
        glPopMatrix();
    }
}