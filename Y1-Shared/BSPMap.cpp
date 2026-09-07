#include "BSPMap.h"
#include "BSPReader.h"
#include <fstream>
#include <cstdlib>
#include <sstream>

void BSPMap::Reset() {
    m_models.clear();
    m_entities.clear();
    m_planes.clear();
    m_clipnodes.clear();
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
    return num;
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

    {
        int headnode = m_models[0].headnode[hullIndex];
        TraceResult t = TraceHull(headnode, start, end);
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
        TraceResult t = TraceHull(headnode, localStart, localEnd);

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
    if (m_models.empty() || m_clipnodes.empty() || m_planes.empty()) return false;
    if (hullIndex < 0 || hullIndex > 3) hullIndex = 1;

    int headnode = m_models[0].headnode[hullIndex];
    glm::vec3 bspPos = ConvertToBSP(enginePos);
    int contents = HullPointContents(headnode, bspPos);
    return contents == CONTENTS_SOLID;
}