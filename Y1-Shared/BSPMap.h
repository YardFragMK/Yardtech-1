#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "BSPFormat.h"
#include "EntityParser.h"

// Bir trace (raycast/sweep) sonucu, engine-space (Y-up) uzayinda doner.
struct TraceResult {
    bool startSolid = false;
    bool allSolid = true;
    bool inOpen = false;
    bool inWater = false;
    float fraction = 1.0f;
    glm::vec3 endPos{ 0.0f };
    glm::vec3 planeNormal{ 0.0f, 1.0f, 0.0f };
};

// Yalnizca collision ve entity verisiyle ilgilenen, hicbir grafik API'sine
// (OpenGL, Direct3D vb.) bagimliligi olmayan BSP okuyucu. Hem sunucunun
// yetkili fizik simulasyonunda hem istemcinin tahmininde kullanilir. Gorsel
// verilerle (texture, lightmap, gorunum) ilgili her sey ayri bir sinifta
// (istemci tarafinda) yasar.
class BSPMap {
public:
    bool Load(const std::string& bspPath);

    TraceResult TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex = 1) const;
    glm::vec3 SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex = 1) const;
    bool IsPointSolid(const glm::vec3& enginePos, int hullIndex) const;

    const std::vector<Entity>& GetEntities() const { return m_entities; }
    const std::string& GetSkyName() const { return m_skyName; }

    static glm::vec3 ParseOriginToEngineSpace(const std::string& originStr);

private:
    std::vector<BSPModel_t> m_models;
    std::vector<Entity> m_entities;
    std::vector<BSPPlane_t> m_planes;
    std::vector<BSPClipNode_t> m_clipnodes;
    std::string m_skyName;

    static glm::vec3 ConvertCoord(const float p[3]) {
        return glm::vec3(p[0], p[2], -p[1]);
    }
    static glm::vec3 ConvertToEngine(const glm::vec3& bsp) {
        return glm::vec3(bsp.x, bsp.z, -bsp.y);
    }
    static glm::vec3 ConvertToBSP(const glm::vec3& engineSpace) {
        return glm::vec3(engineSpace.x, -engineSpace.z, engineSpace.y);
    }

    static int ParseBrushModelIndex(const std::string& modelStr);

    int HullPointContents(int num, const glm::vec3& p) const;
    bool RecursiveHullCheck(int rootNode, int num, float p1f, float p2f,
        const glm::vec3& p1, const glm::vec3& p2, TraceResult& trace) const;
    TraceResult TraceHull(int headnode, const glm::vec3& start, const glm::vec3& end) const;

    void Reset();
};

extern BSPMap g_Map;