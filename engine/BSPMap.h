#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>
#include "BSPFormat.h"
#include "WadFile.h"
#include "../game/src/EntityParser.h"

// Bir BSP face'inin, dünya uzayina cevrilmis (Y-up) render verisi
struct BSPRenderFace {
    std::vector<glm::vec3> positions; // convex polygon, fan olarak cizilecek
    std::vector<glm::vec2> texcoords;
    GLuint glTexture = 0;
};

class BSPMap {
public:
    // bspPath: .bsp dosyasi. wadSearchDirs: harici wad aranacak klasorler
    // (ornegin {"", "wads/", "textures/"} -> bsp'nin bulundugu klasore gore relative)
    bool Load(const std::string& bspPath, const std::vector<std::string>& wadSearchDirs = { "", "wads/", "textures/" });

    // BeginFrame(camera) sonrasi cagrilmali (MODELVIEW zaten view matrisi)
    void RenderWorld() const;

    // Brush entity'leri (func_wall, func_door vb.) icin: model index = "*N" key'indeki N
    void RenderModel(int modelIndex) const;

    void RenderBrushEntities() const;

    int GetModelCount() const { return static_cast<int>(m_models.size()); }
    const std::string& GetEntityText() const { return m_entityText; }
    const std::vector<Entity>& GetEntities() const { return m_entities; }

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

    // texinfo.miptex index -> GL texture
    std::vector<GLuint> m_textureIdByMiptex;
    std::vector<glm::ivec2> m_textureSizeByMiptex;

    std::vector<WadFile> m_wads;

    // her modelin firstface..firstface+numfaces araligi icin
    // texture'a gore gruplanmis render face listesi
    std::unordered_map<int, std::vector<BSPRenderFace>> m_renderFacesByModel;

    static glm::vec3 ConvertCoord(const float p[3]) {
        // BSP: Z-up sag-el  ->  Motor: Y-up sag-el
        return glm::vec3(p[0], p[2], -p[1]);
    }

    // ayni BSP->engine eksen donusumunu uygular
    static glm::vec3 ParseOriginToEngineSpace(const std::string& originStr);

    static int ParseBrushModelIndex(const std::string& modelStr);

    void BuildTextures(const std::vector<uint8_t>& textureLumpRaw);
    void BuildRenderFaces();
    std::string ExtractWorldspawnWadKey() const;
    void LoadExternalWads(const std::vector<std::string>& wadSearchDirs);
};