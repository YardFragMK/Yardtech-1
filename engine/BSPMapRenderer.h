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
#include "GLExtensions.h"

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

// Bir .bsp dosyasinin gorsel tarafini (texture, lightmap, gorunurluk gridi,
// cizim) yonetir. Collision/entity verisi bu sinifin ilgi alani disindadir --
// onun icin BSPMap (Y1-Shared) kullanilir. Istemci, ayni .bsp dosyasini hem
// BSPMap::Load hem BSPMapRenderer::Load ile ayri ayri acar; bu kucuk bir
// dosya-okuma tekrari pahasina, sunucunun hicbir grafik API'sine bagimli
// olmamasini saglar.
class BSPMapRenderer {
public:
    bool Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs = { "", "wads/", "textures/" });

    void RenderWorld(const Frustum& frustum) const;
    void RenderWorld() const;
    void RenderModel(int modelIndex) const;
    void RenderBrushEntities(const std::vector<Entity>& entities, const Frustum& frustum) const;
    void RenderBrushEntities(const std::vector<Entity>& entities) const;

private:
    std::vector<BSPVertex_t> m_vertices;
    std::vector<BSPEdge_t> m_edges;
    std::vector<BSPSurfEdge_t> m_surfedges;
    std::vector<BSPFace_t> m_faces;
    std::vector<BSPTexInfo_t> m_texinfos;
    std::vector<BSPModel_t> m_models;
    std::string m_entityText;
    std::string m_bspDir;

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

    static glm::vec3 ConvertCoord(const float p[3]) {
        return glm::vec3(p[0], p[2], -p[1]);
    }
    static void ConvertAABB(const float bspMins[3], const float bspMaxs[3],
        glm::vec3& outMins, glm::vec3& outMaxs) {
        outMins = glm::vec3(bspMins[0], bspMins[2], -bspMaxs[1]);
        outMaxs = glm::vec3(bspMaxs[0], bspMaxs[2], -bspMins[1]);
    }
    static int ParseBrushModelIndex(const std::string& modelStr);
    static glm::vec3 ParseOriginToEngineSpace(const std::string& originStr);

    void BuildTextures(const std::vector<uint8_t>& textureLumpRaw);
    void BuildRenderFaces();
    std::string ExtractWorldspawnWadKey() const;
    void LoadExternalWads(const std::vector<std::string>& wadSearchDirs);

    static void ComputeFaceAABB(const BSPRenderFace& rf, glm::vec3& outMins, glm::vec3& outMaxs);
    int GetOrCreateCell(const glm::vec3& faceCenter, std::unordered_map<long long, int>& cellIndexMap);

    void Reset();
};

extern BSPMapRenderer g_MapRenderer;