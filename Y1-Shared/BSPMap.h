#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "BSPFormat.h"
#include "EntityParser.h"
#include "iworldquery.h"

// Bir noktanin icinde bulundugu sivi turu (BSP yaprak icerigine gore).
enum LiquidKind {
    LIQUID_NONE = 0,
    LIQUID_WATER = 1,
    LIQUID_SLIME = 2,
    LIQUID_LAVA = 3
};

// Yalnizca collision ve entity verisiyle ilgilenen, hicbir grafik API'sine
// (OpenGL, Direct3D vb.) bagimliligi olmayan BSP okuyucu. Hem sunucunun
// yetkili fizik simulasyonunda hem istemcinin tahmininde kullanilir. Gorsel
// verilerle (texture, lightmap, gorunum) ilgili her sey ayri bir sinifta
// (istemci tarafinda) yasar.
class BSPMap {
public:
    bool Load(const std::string& bspPath);
    bool LoadFromMemory(const std::vector<uint8_t>& bspData);

    // hullIndex 0: nokta hull'u (dugum + yaprak agacindan kurulur), 1..3: clipnode hull'lari.
    TraceResult TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex = 1) const;
    glm::vec3 SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex = 1) const;
    bool IsPointSolid(const glm::vec3& enginePos, int hullIndex) const;

    // Noktanin worldspawn icindeki sivi hacminde olup olmadigini dondurur
    // (LiquidKind). Brush entity'ler (func_water vb.) dahil degildir.
    int GetLiquidAt(const glm::vec3& enginePos) const;

    const std::vector<Entity>& GetEntities() const { return m_entities; }
    const std::string& GetSkyName() const { return m_skyName; }

    static glm::vec3 ParseOriginToEngineSpace(const std::string& originStr);

private:
    std::vector<BSPModel_t> m_models;
    std::vector<Entity> m_entities;
    std::vector<BSPPlane_t> m_planes;
    std::vector<BSPClipNode_t> m_clipnodes;
    std::vector<BSPClipNode_t> m_hull0Nodes; // nokta hull'u: dugumlerden, yaprak icerikleriyle kurulur
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

    int HullPointContents(const std::vector<BSPClipNode_t>& nodes, int num, const glm::vec3& p) const;
    bool RecursiveHullCheck(const std::vector<BSPClipNode_t>& nodes, int rootNode, int num, float p1f, float p2f,
        const glm::vec3& p1, const glm::vec3& p2, TraceResult& trace) const;
    TraceResult TraceHull(const std::vector<BSPClipNode_t>& nodes, int headnode,
        const glm::vec3& start, const glm::vec3& end) const;

    void Reset();
};

extern BSPMap g_Map;