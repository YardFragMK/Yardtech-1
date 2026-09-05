#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>
#include "BSPFormat.h"
#include "WadFile.h"
#include "EntityParser.h"
#include "Frustum.h"
#include "../engine/GLExtensions.h"

struct BSPRenderFace {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec2> lightUVs;   
    GLuint glTexture = 0;
    GLuint glLightmap = 0; 
    bool isMasked = false;
    bool isSky = false;
};

struct WorldGridCell {
    glm::vec3 mins{ 0.0f };
    glm::vec3 maxs{ 0.0f };
    std::vector<BSPRenderFace> faces;
};

struct TraceResult {
    bool startSolid = false;
    bool allSolid = true;
    bool inOpen = false;
    bool inWater = false;
    float fraction = 1.0f;
    glm::vec3 endPos{ 0.0f };
    glm::vec3 planeNormal{ 0.0f, 1.0f, 0.0f };
};

class BSPMap {
public:
    bool Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs = { "", "wads/", "textures/" });

    void RenderWorld(const Frustum& frustum) const;
    void RenderWorld() const;
    void RenderModel(int modelIndex) const;
    void RenderBrushEntities(const Frustum& frustum) const;
    void RenderBrushEntities() const;

    TraceResult TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex = 1) const;

    int GetModelCount() const { return static_cast<int>(m_models.size()); }
    const std::string& GetEntityText() const { return m_entityText; }
    const std::vector<Entity>& GetEntities() const { return m_entities; }
    glm::vec3 SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex = 1) const;
    static glm::vec3 ParseOriginToEngineSpace(const std::string& originStr);
    bool IsPointSolid(const glm::vec3& enginePos, int hullIndex) const;
    void LoadMap();
    const std::string& GetSkyName() const { return m_skyName; }

private:
    std::string m_bspDir;

    std::vector<BSPVertex_t> m_vertices;
    std::vector<BSPEdge_t> m_edges;
    std::vector<BSPSurfEdge_t> m_surfedges;
    std::vector<BSPFace_t> m_faces;
    std::vector<BSPTexInfo_t> m_texinfos;
    std::vector<BSPModel_t> m_models;
    std::string m_entityText;
    std::vector<Entity> m_entities;

    std::vector<BSPPlane_t> m_planes;
    std::vector<BSPClipNode_t> m_clipnodes;

    std::vector<GLuint> m_textureIdByMiptex;
    std::vector<glm::ivec2> m_textureSizeByMiptex;
    std::vector<bool> m_isMaskedByMiptex;

    std::vector<uint8_t> m_lightingData; 

    std::vector<WadFile> m_wads;

    std::unordered_map<int, std::vector<BSPRenderFace>> m_renderFacesByModel;

    std::vector<WorldGridCell> m_worldCells;
    static constexpr float GRID_CELL_SIZE = 512.0f;

    std::vector<glm::vec3> m_modelAABBMins;
    std::vector<glm::vec3> m_modelAABBMaxs;
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

    static void ConvertAABB(const float bspMins[3], const float bspMaxs[3],
        glm::vec3& outMins, glm::vec3& outMaxs) {
        outMins = glm::vec3(bspMins[0], bspMins[2], -bspMaxs[1]);
        outMaxs = glm::vec3(bspMaxs[0], bspMaxs[2], -bspMins[1]);
    }

    static int ParseBrushModelIndex(const std::string& modelStr);

    void BuildTextures(const std::vector<uint8_t>& textureLumpRaw);
    void BuildRenderFaces();
    std::string ExtractWorldspawnWadKey() const;
    void LoadExternalWads(const std::vector<std::string>& wadSearchDirs);

    static void ComputeFaceAABB(const BSPRenderFace& rf, glm::vec3& outMins, glm::vec3& outMaxs);
    int GetOrCreateCell(const glm::vec3& faceCenter, std::unordered_map<long long, int>& cellIndexMap);

    int HullPointContents(int num, const glm::vec3& p) const;
    bool RecursiveHullCheck(int rootNode, int num, float p1f, float p2f,
        const glm::vec3& p1, const glm::vec3& p2, TraceResult& trace) const;
    TraceResult TraceHull(int headnode, const glm::vec3& start, const glm::vec3& end) const;
    void Reset();
};

extern BSPMap g_Map;