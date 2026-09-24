#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <RTGL1.h>
#include "BSPFormat.h"
#include "WadFile.h"
#include "EntityParser.h"
#include "Frustum.h"

// Bir yuzeyin (BSP face) onceden hesaplanmis, RTGL1'e her frame yuklenmeye
// hazir hali. Fan-ucgenlenmis vertex/index dizileri build-time'da (BuildRenderFaces
// icinde) bir kez hesaplanir; RenderWorld/RenderModel her frame bu diziyi
// rgUploadMeshPrimitive'e verir -- RTGL1'in kendisi, statik geometrinin
// her frame yeniden "upload" edilmesini bekleyen bir API tasarimina sahip
// (klasik retained-mode degil, per-frame immediate-benzeri bir model).
struct BSPRenderFace {
    std::vector<RgPrimitiveVertex> vertices;
    std::vector<uint32_t> indices;
    std::string textureName; // rgProvideOriginalTexture ile kayitli isim; bos ise texture yok/desteklenmiyor
    bool isMasked = false;
    bool isSky = false;
    uint32_t uniqueObjectID = 0; // build-time'da atanir, frame'ler arasi sabit kalir
};

struct WorldGridCell {
    glm::vec3 mins{ 0.0f };
    glm::vec3 maxs{ 0.0f };
    std::vector<BSPRenderFace> faces;
};

// Bir .bsp dosyasinin gorsel tarafini (texture, gorunurluk gridi, RTGL1'e
// geometri/texture yukleme) yonetir. Collision/entity verisi bu sinifin
// ilgi alani disindadir -- onun icin BSPMap (Y1-Shared) kullanilir.
class BSPMapRenderer {
public:
    bool Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs = { "", "wads/", "textures/" });

    // RTGL1'e bu frame'in dunya geometrisini yukler (rgStartFrame ile
    // rgDrawFrame arasinda cagrilmalidir). Cizim RTGL1'in kendi ic
    // pipeline'inda gerceklesir, bu fonksiyon sadece veri "upload" eder.
    void RenderWorld(const Frustum& frustum) const;
    void RenderWorld() const;
    void RenderModel(int modelIndex, const glm::vec3& origin) const;
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

    std::vector<std::string> m_textureNameByMiptex; // bos string = texture yok/desteklenmiyor
    std::vector<bool> m_isMaskedByMiptex;
    std::vector<WadFile> m_wads;

    std::unordered_map<int, std::vector<BSPRenderFace>> m_renderFacesByModel;
    std::vector<WorldGridCell> m_worldCells;
    static constexpr float GRID_CELL_SIZE = 512.0f;

    std::vector<glm::vec3> m_modelAABBMins;
    std::vector<glm::vec3> m_modelAABBMaxs;

    uint32_t m_nextObjectID = 1; // 0'i "atanmamis" olarak ayirmak icin 1'den basliyor

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

    void UploadFace(const BSPRenderFace& rf, const char* meshName, const RgTransform& transform) const;

    void Reset();
};

extern BSPMapRenderer g_MapRenderer;