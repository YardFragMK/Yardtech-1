#include "BSPMap.h"
#include "BSPReader.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include "console/Console.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include "Engine.h"

void BSPMap::Reset() {
    // GL texture'lari sil (miptex'ler)
    for (GLuint tex : m_textureIdByMiptex) {
        if (tex != 0) glDeleteTextures(1, &tex);
    }

    // WadFile'lar kendi texture'larini yonetiyor ama onlarin da GL kaynaklarini
    // WadFile destructor'i silmiyor su an (bilinen bir eksik) -- burada ekstra
    // temizlik yapmiyoruz, WadFile'a kendi Unload'unu eklemek ayri bir is.

    m_bspDir.clear();
    m_vertices.clear();
    m_edges.clear();
    m_surfedges.clear();
    m_faces.clear();
    m_texinfos.clear();
    m_models.clear();
    m_entityText.clear();
    m_entities.clear();
    m_planes.clear();
    m_clipnodes.clear();
    m_textureIdByMiptex.clear();
    m_textureSizeByMiptex.clear();
    m_wads.clear();
    m_renderFacesByModel.clear();
    m_worldCells.clear();
    m_modelAABBMins.clear();
    m_modelAABBMaxs.clear();
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

bool BSPMap::Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs) {
    Reset();

    std::ifstream file(bspPath, std::ios::binary);
    if (!file) {
        Console::Log("WARNING-> bsp acilamadi: " + bspPath + "NO:1");
        return false;
    }

    size_t slash = bspPath.find_last_of("/\\");
    m_bspDir = (slash == std::string::npos) ? "" : bspPath.substr(0, slash + 1);

    BSPHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(BSPHeader));
    if (header.version != 30) {
        Console::Log("WARNING-> bsp versiyonu uyumlu degil (" + std::to_string(header.version) + ")");
    }

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
    m_planes = ReadLump<BSPPlane_t>(file, header.lumps[LUMP_PLANES]);
    m_clipnodes = ReadLump<BSPClipNode_t>(file, header.lumps[LUMP_CLIPNODES]);

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

    // model AABB'lerini onceden cikar (RenderBrushEntities culling icin)
    m_modelAABBMins.resize(m_models.size());
    m_modelAABBMaxs.resize(m_models.size());
    for (size_t i = 0; i < m_models.size(); i++) {
        ConvertAABB(m_models[i].mins, m_models[i].maxs, m_modelAABBMins[i], m_modelAABBMaxs[i]);
    }

    BuildRenderFaces();

    Console::Log(bspPath + " yuklendi: " + std::to_string(m_models.size()) + " model, " +
        std::to_string(m_faces.size()) + " face, " +
        std::to_string(m_worldCells.size()) + " world grid hucresi");

    // --- hull tanisi ---
    if (!m_models.empty()) {
        Console::Log("Model0 headnodes: hull0=" + std::to_string(m_models[0].headnode[0])
            + " hull1=" + std::to_string(m_models[0].headnode[1])
            + " hull2=" + std::to_string(m_models[0].headnode[2])
            + " hull3=" + std::to_string(m_models[0].headnode[3])
            + " (clipnode sayisi=" + std::to_string(m_clipnodes.size()) + ")");
    }
    return true;
}

std::string BSPMap::ExtractWorldspawnWadKey() const {
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

void BSPMap::ComputeFaceAABB(const BSPRenderFace& rf, glm::vec3& outMins, glm::vec3& outMaxs) {
    outMins = glm::vec3(FLT_MAX);
    outMaxs = glm::vec3(-FLT_MAX);
    for (const auto& p : rf.positions) {
        if (p.x < outMins.x) outMins.x = p.x;
        if (p.y < outMins.y) outMins.y = p.y;
        if (p.z < outMins.z) outMins.z = p.z;

        if (p.x > outMaxs.x) outMaxs.x = p.x;
        if (p.y > outMaxs.y) outMaxs.y = p.y;
        if (p.z > outMaxs.z) outMaxs.z = p.z;
    }
}

int BSPMap::GetOrCreateCell(const glm::vec3& faceCenter, std::unordered_map<long long, int>& cellIndexMap) {
    int cx = static_cast<int>(std::floor(faceCenter.x / GRID_CELL_SIZE));
    int cy = static_cast<int>(std::floor(faceCenter.y / GRID_CELL_SIZE));
    int cz = static_cast<int>(std::floor(faceCenter.z / GRID_CELL_SIZE));

    // 3 int'i tek 64-bit anahtara paketle (harita boyutu icin yeterli aralik)
    long long key =
        (static_cast<long long>(cx + 100000) << 42) ^
        (static_cast<long long>(cy + 100000) << 21) ^
        (static_cast<long long>(cz + 100000));

    auto it = cellIndexMap.find(key);
    if (it != cellIndexMap.end()) {
        return it->second;
    }

    WorldGridCell cell;
    cell.mins = glm::vec3(cx * GRID_CELL_SIZE, cy * GRID_CELL_SIZE, cz * GRID_CELL_SIZE);
    cell.maxs = cell.mins + glm::vec3(GRID_CELL_SIZE);

    m_worldCells.push_back(std::move(cell));
    int idx = static_cast<int>(m_worldCells.size()) - 1;
    cellIndexMap[key] = idx;
    return idx;
}

void BSPMap::BuildRenderFaces() {
    std::unordered_map<long long, int> cellIndexMap;

    for (size_t m = 0; m < m_models.size(); m++) {
        const BSPModel_t& model = m_models[m];

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

            if (rf.positions.size() < 3) continue;

            if (m == 0) {
                // worldspawn: grid hucresine bucket'la
                glm::vec3 faceCenter(0.0f);
                for (const auto& p : rf.positions) faceCenter += p;
                faceCenter /= static_cast<float>(rf.positions.size());

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

static void DrawRenderFace(const BSPRenderFace& rf) {
    if (rf.positions.size() < 3) return;

    if (rf.glTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, rf.glTexture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    }

    glBegin(GL_POLYGON);
    for (size_t i = 0; i < rf.positions.size(); i++) {
        glTexCoord2f(rf.texcoords[i].x, rf.texcoords[i].y);
        glVertex3f(rf.positions[i].x, rf.positions[i].y, rf.positions[i].z);
    }
    glEnd();

    if (rf.glTexture == 0) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void BSPMap::RenderWorld() const {
    glEnable(GL_TEXTURE_2D);
    for (const auto& cell : m_worldCells) {
        for (const auto& rf : cell.faces) {
            DrawRenderFace(rf);
        }
    }
    glDisable(GL_TEXTURE_2D);
}

void BSPMap::RenderWorld(const Frustum& frustum) const {
    glEnable(GL_TEXTURE_2D);
    for (const auto& cell : m_worldCells) {
        if (!frustum.IntersectsAABB(cell.mins, cell.maxs)) continue;
        for (const auto& rf : cell.faces) {
            DrawRenderFace(rf);
        }
    }
    glDisable(GL_TEXTURE_2D);
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
        if (modelIndex <= 0) continue;

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

void BSPMap::RenderBrushEntities(const Frustum& frustum) const {
    for (const Entity& ent : m_entities) {
        const std::string* modelKey = ent.Get(EntityKeys::Model);
        if (!modelKey) continue;

        int modelIndex = ParseBrushModelIndex(*modelKey);
        if (modelIndex <= 0) continue;
        if (modelIndex >= static_cast<int>(m_modelAABBMins.size())) continue;

        glm::vec3 origin(0.0f);
        if (const std::string* originKey = ent.Get(EntityKeys::Origin)) {
            origin = ParseOriginToEngineSpace(*originKey);
        }

        // model'in lokal AABB'sine origin'i ekleyip dunya-uzayi AABB'sini bul
        glm::vec3 worldMins = m_modelAABBMins[modelIndex] + origin;
        glm::vec3 worldMaxs = m_modelAABBMaxs[modelIndex] + origin;

        if (!frustum.IntersectsAABB(worldMins, worldMaxs)) continue;

        glPushMatrix();
        glTranslatef(origin.x, origin.y, origin.z);
        RenderModel(modelIndex);
        glPopMatrix();
    }
}

int BSPMap::HullPointContents(int num, const glm::vec3& p) const {
    while (num >= 0) {
        const BSPClipNode_t& node = m_clipnodes[num];
        const BSPPlane_t& plane = m_planes[node.planenum];

        float d;
        if (plane.type < 3) {
            d = p[plane.type] - plane.dist;
        }
        else {
            d = plane.normal[0] * p.x + plane.normal[1] * p.y + plane.normal[2] * p.z - plane.dist;
        }

        num = (d < 0.0f) ? node.children[1] : node.children[0];
    }
    return num; // CONTENTS_* (negatif)
}

bool BSPMap::RecursiveHullCheck(int rootNode, int num, float p1f, float p2f,
    const glm::vec3& p1, const glm::vec3& p2, TraceResult& trace) const {
    if (num < 0) {
        if (num != CONTENTS_SOLID) {
            trace.allSolid = false;
            if (num == CONTENTS_EMPTY) trace.inOpen = true;
            else trace.inWater = true;
        }
        else {
            trace.startSolid = true;
        }
        return true;
    }

    const BSPClipNode_t& node = m_clipnodes[num];
    const BSPPlane_t& plane = m_planes[node.planenum];

    float t1, t2;
    if (plane.type < 3) {
        t1 = p1[plane.type] - plane.dist;
        t2 = p2[plane.type] - plane.dist;
    }
    else {
        t1 = plane.normal[0] * p1.x + plane.normal[1] * p1.y + plane.normal[2] * p1.z - plane.dist;
        t2 = plane.normal[0] * p2.x + plane.normal[1] * p2.y + plane.normal[2] * p2.z - plane.dist;
    }

    if (t1 >= 0.0f && t2 >= 0.0f)
        return RecursiveHullCheck(rootNode, node.children[0], p1f, p2f, p1, p2, trace);
    if (t1 < 0.0f && t2 < 0.0f)
        return RecursiveHullCheck(rootNode, node.children[1], p1f, p2f, p1, p2, trace);

    int side = (t1 < 0.0f) ? 1 : 0;
    const float DIST_EPSILON = 0.03125f;
    float frac = (t1 < 0.0f) ? (t1 + DIST_EPSILON) / (t1 - t2) : (t1 - DIST_EPSILON) / (t1 - t2);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    float midf = p1f + (p2f - p1f) * frac;
    glm::vec3 mid = p1 + (p2 - p1) * frac;

    if (!RecursiveHullCheck(rootNode, node.children[side], p1f, midf, p1, mid, trace))
        return false;

    if (HullPointContents(node.children[side ^ 1], mid) != CONTENTS_SOLID)
        return RecursiveHullCheck(rootNode, node.children[side ^ 1], midf, p2f, mid, p2, trace);

    if (trace.allSolid)
        return false;

    if (side == 0) {
        trace.planeNormal = glm::vec3(plane.normal[0], plane.normal[1], plane.normal[2]);
    }
    else {
        trace.planeNormal = -glm::vec3(plane.normal[0], plane.normal[1], plane.normal[2]);
    }

    // epsilon kaymasi telafisi: carpisma noktasindan geri cekilerek gercek yuzeyi bul
    while (HullPointContents(rootNode, mid) == CONTENTS_SOLID) {
        frac -= 0.1f;
        if (frac < 0.0f) {
            trace.fraction = midf;
            trace.endPos = mid;
            return false;
        }
        midf = p1f + (p2f - p1f) * frac;
        mid = p1 + (p2 - p1) * frac;
    }

    trace.fraction = midf;
    trace.endPos = mid;
    return false;
}

TraceResult BSPMap::TraceHull(int headnode, const glm::vec3& start, const glm::vec3& end) const {
    TraceResult trace;
    trace.fraction = 1.0f;
    trace.allSolid = true;

    glm::vec3 bspStart = ConvertToBSP(start);
    glm::vec3 bspEnd = ConvertToBSP(end);
    trace.endPos = bspEnd;

    RecursiveHullCheck(headnode, headnode, 0.0f, 1.0f, bspStart, bspEnd, trace);

    if (trace.fraction >= 1.0f) {
        trace.endPos = end;
    }
    else {
        trace.endPos = ConvertToEngine(trace.endPos);
    }
    trace.planeNormal = ConvertToEngine(trace.planeNormal);
    return trace;
}

TraceResult BSPMap::TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex) const {
    TraceResult best;
    best.fraction = 1.0f;
    best.endPos = end;

    if (m_models.empty() || m_clipnodes.empty() || m_planes.empty()) {
        return best;
    }
    if (hullIndex < 0 || hullIndex > 3) hullIndex = 1;

    // --- worldspawn (model 0) ---
    {
        int headnode = m_models[0].headnode[hullIndex];
        TraceResult t = TraceHull(headnode, start, end);
        if (t.fraction < best.fraction) best = t;
    }

    // --- brush entity'ler (func_wall, func_door, vb.) ---
    for (const Entity& ent : m_entities) {
        const std::string* modelKey = ent.Get(EntityKeys::Model);
        if (!modelKey) continue;

        int modelIndex = ParseBrushModelIndex(*modelKey);
        if (modelIndex <= 0 || modelIndex >= static_cast<int>(m_models.size())) continue;

        // Gorsel olarak var ama collision'i olmamasi gereken tipik entity'leri atla.
        const std::string* cn = ent.Get(EntityKeys::Classname);
        if (cn) {
            if (*cn == "func_illusionary") continue;
            if (cn->rfind("trigger_", 0) == 0) continue;
        }

        glm::vec3 origin(0.0f);
        if (const std::string* originKey = ent.Get(EntityKeys::Origin)) {
            origin = ParseOriginToEngineSpace(*originKey);
        }

        // Model'in hull'u kendi lokal (origin uygulanmamis) uzayinda tanimli,
        // o yuzden start/end'i origin kadar geri kaydirip test ediyoruz.
        glm::vec3 localStart = start - origin;
        glm::vec3 localEnd = end - origin;

        int headnode = m_models[modelIndex].headnode[hullIndex];
        TraceResult t = TraceHull(headnode, localStart, localEnd);

        if (t.fraction < best.fraction) {
            t.endPos += origin; // sonucu tekrar dunya uzayina tasi
            best = t;
        }
    }

    return best;
}

static glm::vec3 ClipVelocity(const glm::vec3& in, const glm::vec3& normal, float overbounce) {
    float backoff = glm::dot(in, normal) * overbounce;
    glm::vec3 out = in - normal * backoff;

    // kucuk kalintilari sifirla, titremeyi (jitter) onler
    if (glm::length(out) < 0.001f) return glm::vec3(0.0f);
    return out;
}

glm::vec3 BSPMap::SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex) const {
    glm::vec3 current = start;
    glm::vec3 remaining = end - start;

    if (glm::length(remaining) < 0.0001f) return start;

    // Cok uzun tek-frame hareketlerinde nadir sayisal kacaklari azaltmak icin
    // hareketi 64 unit'lik alt-adimlara bol.
    const float MAX_STEP = 64.0f;
    float totalLen = glm::length(remaining);
    int subSteps = static_cast<int>(std::ceil(totalLen / MAX_STEP));
    if (subSteps < 1) subSteps = 1;
    if (subSteps > 8) subSteps = 8; // guvenlik siniri

    glm::vec3 stepVec = remaining / static_cast<float>(subSteps);

    const int MAX_BUMPS = 4;
    std::vector<glm::vec3> planeNormals;
    planeNormals.reserve(MAX_BUMPS);

    for (int step = 0; step < subSteps; step++) {
        glm::vec3 stepRemaining = stepVec;
        planeNormals.clear();

        for (int bump = 0; bump < MAX_BUMPS; bump++) {
            if (glm::length(stepRemaining) < 0.0001f) break;

            glm::vec3 target = current + stepRemaining;
            TraceResult trace = TraceLine(current, target, hullIndex);

            if (trace.fraction >= 1.0f) {
                current = target;
                break;
            }

            // trace'in ulastigi noktaya kadar git
            glm::vec3 newCurrent = trace.endPos;

            // bu bump'ta gidilemeyen kisim
            glm::vec3 unresolved = target - newCurrent;
            current = newCurrent;

            planeNormals.push_back(trace.planeNormal);

            // kalan hareketi, simdiye kadar carpilan TUM duzlemlere gore clip et
            glm::vec3 clipped = unresolved;
            for (const auto& n : planeNormals) {
                clipped = ClipVelocity(clipped, n, 1.0f);
            }
            stepRemaining = clipped;

            // clip sonrasi hareket, orijinal yone ters donduyse (koseye sikismis)
            // dur, daha fazla ilerlemeye calisma
            if (glm::dot(stepRemaining, stepVec) < 0.0f) {
                stepRemaining = glm::vec3(0.0f);
                break;
            }
        }
    }

    return current;
}

bool BSPMap::IsPointSolid(const glm::vec3& enginePos, int hullIndex) const {
    if (m_models.empty() || m_clipnodes.empty() || m_planes.empty()) return false;
    if (hullIndex < 0 || hullIndex > 3) hullIndex = 1;

    int headnode = m_models[0].headnode[hullIndex];
    glm::vec3 bspPos = ConvertToBSP(enginePos);
    int contents = HullPointContents(headnode, bspPos);
    return contents == CONTENTS_SOLID;
}

void BSPMap::LoadMap() {
    if (!g_Map.Load("nvs1/map/cs_assault.bsp", { "", "map/", "wads/", "textures/" })) {
        Logger::error("BSP yuklenemedi.");
        // return false;
    }


    // --- player_start'tan spawn ---
    {
        bool foundStart = false;
        for (const Entity& ent : g_Map.GetEntities()) {
            if (ent.Is(EntityClassnames::PlayerStart)) {
                if (const std::string* originStr = ent.Get(EntityKeys::Origin)) {
                    glm::vec3 spawnPos = BSPMap::ParseOriginToEngineSpace(*originStr);
                    spawnPos.y += 36.0f; // player_start origin genelde ayak hizasinda; goz hizasina tasi
                    g_Camera.position = spawnPos;
                    foundStart = true;
                }

                if (const std::string* angleStr = ent.Get(EntityKeys::Angle)) {
                    float angle = std::atof(angleStr->c_str());
                    g_Camera.yaw = angle; // GoldSrc'de angle = derece cinsinden yaw
                }
                break;
            }
        }
        if (!foundStart) {
            Logger::error("player_start bulunamadi, varsayilan konumdan spawn ediliyor.");
        }
    }
}