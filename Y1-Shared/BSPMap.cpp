#include "BSPMap.h"
#include "BSPReader.h"
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <sstream>

namespace {
    // Diskteki dugum ve yaprak kayitlari. BSPFormat.h'a bagimli olmamak icin burada
    // (GoldSrc BSP v30 duzeniyle) tanimlidir.
#pragma pack(push, 1)
    struct DiskNode {
        int32_t planenum;
        int16_t children[2]; // >= 0: dugum indeksi, < 0: -1 - yaprak indeksi
        int16_t mins[3];
        int16_t maxs[3];
        uint16_t firstface;
        uint16_t numfaces;
    };

    struct DiskLeaf {
        int32_t contents;
        int32_t visofs;
        int16_t mins[3];
        int16_t maxs[3];
        uint16_t firstmarksurface;
        uint16_t nummarksurfaces;
        uint8_t ambientLevel[4];
    };
#pragma pack(pop)

    static_assert(sizeof(DiskNode) == 24, "DiskNode boyutu BSP v30 ile uyusmuyor");
    static_assert(sizeof(DiskLeaf) == 28, "DiskLeaf boyutu BSP v30 ile uyusmuyor");

    // GoldSrc lump sirasi: 0 entities, 1 planes, 2 textures, 3 vertexes, 4 visibility,
    // 5 nodes, 6 texinfo, 7 faces, 8 lighting, 9 clipnodes, 10 leafs, ...
    constexpr int LUMP_INDEX_NODES = 5;
    constexpr int LUMP_INDEX_LEAFS = 10;

    // Yaprak icerik degerleri (Quake/GoldSrc).
    constexpr int kContentsWater = -3;
    constexpr int kContentsSlime = -4;
    constexpr int kContentsLava = -5;
    constexpr int kContentsCurrentFirst = -9;  // -9 .. -14: su akintilari
    constexpr int kContentsCurrentLast = -14;

    // Quake'in Mod_MakeHull0 yontemi: dugum agaci, yapraklari "icerik" degeri olan
    // bir clipnode agacina cevrilir; boylece nokta hull'u diger hull'larla ayni
    // kodla (HullPointContents/RecursiveHullCheck) gezilebilir.
    std::vector<BSPClipNode_t> MakeHull0(const std::vector<DiskNode>& nodes, const std::vector<DiskLeaf>& leafs) {
        std::vector<BSPClipNode_t> hull(nodes.size());

        for (size_t i = 0; i < nodes.size(); i++) {
            hull[i].planenum = nodes[i].planenum;

            for (int side = 0; side < 2; side++) {
                const int child = nodes[i].children[side];
                if (child >= 0) {
                    hull[i].children[side] = static_cast<int16_t>(child);
                }
                else {
                    const size_t leafIndex = static_cast<size_t>(-1 - child);
                    const int contents = (leafIndex < leafs.size()) ? leafs[leafIndex].contents : CONTENTS_SOLID;
                    hull[i].children[side] = static_cast<int16_t>(contents);
                }
            }
        }
        return hull;
    }
}

void BSPMap::Reset() {
    m_models.clear();
    m_entities.clear();
    m_planes.clear();
    m_clipnodes.clear();
    m_hull0Nodes.clear();
    m_skyName.clear();
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

bool BSPMap::Load(const std::string& bspPath) {
    Reset();

    std::ifstream file(bspPath, std::ios::binary);
    if (!file) {
        return false;
    }

    BSPHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(BSPHeader));

    std::string entityText;
    {
        const BSPLump& l = header.lumps[LUMP_ENTITIES_F];
        entityText.assign(static_cast<size_t>(l.length), '\0');
        file.seekg(l.offset, std::ios::beg);
        file.read(entityText.data(), l.length);
    }

    m_entities = ParseEntities(entityText);
    for (const Entity& ent : m_entities) {
        if (ent.Is(EntityClassnames::Worldspawn)) {
            if (const std::string* sky = ent.Get("skyname")) {
                m_skyName = *sky;
            }
            break;
        }
    }

    m_models = ReadLump<BSPModel_t>(file, header.lumps[LUMP_MODELS]);
    m_planes = ReadLump<BSPPlane_t>(file, header.lumps[LUMP_PLANES]);
    m_clipnodes = ReadLump<BSPClipNode_t>(file, header.lumps[LUMP_CLIPNODES]);

    const std::vector<DiskNode> nodes = ReadLump<DiskNode>(file, header.lumps[LUMP_INDEX_NODES]);
    const std::vector<DiskLeaf> leafs = ReadLump<DiskLeaf>(file, header.lumps[LUMP_INDEX_LEAFS]);
    m_hull0Nodes = MakeHull0(nodes, leafs);

    return true;
}



glm::vec3 BSPMap::ParseOriginToEngineSpace(const std::string& originStr) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    std::istringstream ss(originStr);
    ss >> x >> y >> z;
    float p[3] = { x, y, z };
    return ConvertCoord(p);
}

int BSPMap::ParseBrushModelIndex(const std::string& modelStr) {
    if (modelStr.empty() || modelStr[0] != '*') return -1;
    return std::atoi(modelStr.c_str() + 1);
}

int BSPMap::HullPointContents(const std::vector<BSPClipNode_t>& nodes, int num, const glm::vec3& p) const {
    while (num >= 0) {
        // Bozuk/kotu niyetli haritalara karsi sinir denetimi.
        if (static_cast<size_t>(num) >= nodes.size()) return CONTENTS_EMPTY;

        const BSPClipNode_t& node = nodes[num];
        if (node.planenum < 0 || static_cast<size_t>(node.planenum) >= m_planes.size()) return CONTENTS_EMPTY;

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
    return num;
}

bool BSPMap::RecursiveHullCheck(const std::vector<BSPClipNode_t>& nodes, int rootNode, int num, float p1f, float p2f,
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

    // Gecersiz dugum/duzlem indeksi: acik alan sayilir, iz surdurulur.
    if (static_cast<size_t>(num) >= nodes.size() ||
        nodes[num].planenum < 0 || static_cast<size_t>(nodes[num].planenum) >= m_planes.size()) {
        trace.allSolid = false;
        trace.inOpen = true;
        return true;
    }

    const BSPClipNode_t& node = nodes[num];
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
        return RecursiveHullCheck(nodes, rootNode, node.children[0], p1f, p2f, p1, p2, trace);
    if (t1 < 0.0f && t2 < 0.0f)
        return RecursiveHullCheck(nodes, rootNode, node.children[1], p1f, p2f, p1, p2, trace);

    int side = (t1 < 0.0f) ? 1 : 0;
    const float DIST_EPSILON = 0.03125f;
    float frac = (t1 < 0.0f) ? (t1 + DIST_EPSILON) / (t1 - t2) : (t1 - DIST_EPSILON) / (t1 - t2);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    float midf = p1f + (p2f - p1f) * frac;
    glm::vec3 mid = p1 + (p2 - p1) * frac;

    if (!RecursiveHullCheck(nodes, rootNode, node.children[side], p1f, midf, p1, mid, trace))
        return false;

    if (HullPointContents(nodes, node.children[side ^ 1], mid) != CONTENTS_SOLID)
        return RecursiveHullCheck(nodes, rootNode, node.children[side ^ 1], midf, p2f, mid, p2, trace);

    if (trace.allSolid)
        return false;

    if (side == 0) {
        trace.planeNormal = glm::vec3(plane.normal[0], plane.normal[1], plane.normal[2]);
    }
    else {
        trace.planeNormal = -glm::vec3(plane.normal[0], plane.normal[1], plane.normal[2]);
    }

    while (HullPointContents(nodes, rootNode, mid) == CONTENTS_SOLID) {
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

TraceResult BSPMap::TraceHull(const std::vector<BSPClipNode_t>& nodes, int headnode,
    const glm::vec3& start, const glm::vec3& end) const {
    TraceResult trace;
    trace.fraction = 1.0f;
    trace.allSolid = true;

    glm::vec3 bspStart = ConvertToBSP(start);
    glm::vec3 bspEnd = ConvertToBSP(end);
    trace.endPos = bspEnd;

    RecursiveHullCheck(nodes, headnode, headnode, 0.0f, 1.0f, bspStart, bspEnd, trace);

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

    if (hullIndex < 0 || hullIndex > 3) hullIndex = 1;
    // Nokta hull'u kurulamadiysa (dugum/yaprak lump'i yoksa) en kucuk clipnode hull'una dusulur.
    if (hullIndex == 0 && m_hull0Nodes.empty()) hullIndex = 1;

    const std::vector<BSPClipNode_t>& nodes = (hullIndex == 0) ? m_hull0Nodes : m_clipnodes;
    if (m_models.empty() || nodes.empty() || m_planes.empty()) {
        return best;
    }

    {
        int headnode = m_models[0].headnode[hullIndex];
        TraceResult t = TraceHull(nodes, headnode, start, end);
        if (t.fraction < best.fraction) best = t;
    }

    for (const Entity& ent : m_entities) {
        const std::string* modelKey = ent.Get(EntityKeys::Model);
        if (!modelKey) continue;

        int modelIndex = ParseBrushModelIndex(*modelKey);
        if (modelIndex <= 0 || modelIndex >= static_cast<int>(m_models.size())) continue;

        const std::string* cn = ent.Get(EntityKeys::Classname);
        if (cn) {
            if (*cn == "func_illusionary") continue;
            if (cn->rfind("trigger_", 0) == 0) continue;
        }

        glm::vec3 origin(0.0f);
        if (const std::string* originKey = ent.Get(EntityKeys::Origin)) {
            origin = ParseOriginToEngineSpace(*originKey);
        }

        glm::vec3 localStart = start - origin;
        glm::vec3 localEnd = end - origin;

        int headnode = m_models[modelIndex].headnode[hullIndex];
        TraceResult t = TraceHull(nodes, headnode, localStart, localEnd);

        if (t.fraction < best.fraction) {
            t.endPos += origin;
            best = t;
        }
    }

    return best;
}

static glm::vec3 ClipVelocity(const glm::vec3& in, const glm::vec3& normal, float overbounce) {
    float backoff = glm::dot(in, normal) * overbounce;
    glm::vec3 out = in - normal * backoff;

    if (glm::length(out) < 0.001f) return glm::vec3(0.0f);
    return out;
}

glm::vec3 BSPMap::SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex) const {
    glm::vec3 current = start;
    glm::vec3 remaining = end - start;

    if (glm::length(remaining) < 0.0001f) return start;

    const float MAX_STEP = 64.0f;
    float totalLen = glm::length(remaining);
    int subSteps = static_cast<int>(std::ceil(totalLen / MAX_STEP));
    if (subSteps < 1) subSteps = 1;
    if (subSteps > 8) subSteps = 8;

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

            glm::vec3 newCurrent = trace.endPos;
            glm::vec3 unresolved = target - newCurrent;
            current = newCurrent;

            planeNormals.push_back(trace.planeNormal);

            glm::vec3 clipped = unresolved;
            for (const auto& n : planeNormals) {
                clipped = ClipVelocity(clipped, n, 1.0f);
            }
            stepRemaining = clipped;

            if (glm::dot(stepRemaining, stepVec) < 0.0f) {
                stepRemaining = glm::vec3(0.0f);
                break;
            }
        }
    }

    return current;
}

bool BSPMap::IsPointSolid(const glm::vec3& enginePos, int hullIndex) const {
    if (hullIndex < 0 || hullIndex > 3) hullIndex = 1;
    if (hullIndex == 0 && m_hull0Nodes.empty()) hullIndex = 1;

    const std::vector<BSPClipNode_t>& nodes = (hullIndex == 0) ? m_hull0Nodes : m_clipnodes;
    if (m_models.empty() || nodes.empty() || m_planes.empty()) return false;

    int headnode = m_models[0].headnode[hullIndex];
    glm::vec3 bspPos = ConvertToBSP(enginePos);
    int contents = HullPointContents(nodes, headnode, bspPos);
    return contents == CONTENTS_SOLID;
}

int BSPMap::GetLiquidAt(const glm::vec3& enginePos) const {
    if (m_models.empty() || m_hull0Nodes.empty() || m_planes.empty()) return LIQUID_NONE;

    const int headnode = m_models[0].headnode[0];
    const int contents = HullPointContents(m_hull0Nodes, headnode, ConvertToBSP(enginePos));

    if (contents == kContentsWater) return LIQUID_WATER;
    if (contents == kContentsSlime) return LIQUID_SLIME;
    if (contents == kContentsLava) return LIQUID_LAVA;
    if (contents <= kContentsCurrentFirst && contents >= kContentsCurrentLast) return LIQUID_WATER;
    return LIQUID_NONE;
}

template<typename T>
static std::vector<T> ReadLumpFromMemory(const std::vector<uint8_t>& buffer, const BSPLump& lump) {
    std::vector<T> out(lump.length / sizeof(T));
    if (!out.empty() && (lump.offset + lump.length <= buffer.size())) {
        std::memcpy(out.data(), buffer.data() + lump.offset, lump.length);
    }
    return out;
}

bool BSPMap::LoadFromMemory(const std::vector<uint8_t>& bspData) {
    Reset();

    if (bspData.size() < sizeof(BSPHeader)) {
        return false;
    }

    BSPHeader header{};
    std::memcpy(&header, bspData.data(), sizeof(BSPHeader));

    std::string entityText;

    const BSPLump& l = header.lumps[LUMP_ENTITIES_F];

    if (l.offset < 0 || l.length < 0 ||
        static_cast<size_t>(l.offset) + static_cast<size_t>(l.length) > bspData.size()) {
        return false;
    }

    entityText.assign(
        reinterpret_cast<const char*>(bspData.data() + l.offset),
        static_cast<size_t>(l.length)
    );

    m_entities = ParseEntities(entityText);

    for (const Entity& ent : m_entities) {
        if (ent.Is(EntityClassnames::Worldspawn)) {
            if (const std::string* sky = ent.Get("skyname")) {
                m_skyName = *sky;
            }
            break;
        }
    }

    m_models = ReadLumpFromMemory<BSPModel_t>(bspData, header.lumps[LUMP_MODELS]);
    m_planes = ReadLumpFromMemory<BSPPlane_t>(bspData, header.lumps[LUMP_PLANES]);
    m_clipnodes = ReadLumpFromMemory<BSPClipNode_t>(bspData, header.lumps[LUMP_CLIPNODES]);

    const std::vector<DiskNode> nodes = ReadLumpFromMemory<DiskNode>(bspData, header.lumps[LUMP_INDEX_NODES]);
    const std::vector<DiskLeaf> leafs = ReadLumpFromMemory<DiskLeaf>(bspData, header.lumps[LUMP_INDEX_LEAFS]);
    m_hull0Nodes = MakeHull0(nodes, leafs);

    return true;
}