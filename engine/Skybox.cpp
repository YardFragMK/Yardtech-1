#include "Skybox.h"
#include "TGALoader.h"
#include "console/Console.h"
#include "RTInstance.h"
#include <vector>

Skybox g_Skybox;

static std::string UploadSkyTexture(const std::string& path, const std::string& name) {
    std::vector<uint8_t> pixels;
    int w = 0, h = 0;
    if (!LoadTGA(path, pixels, w, h)) return "";

    RgOriginalTextureInfo texInfo{};
    texInfo.pTextureName = name.c_str();
    texInfo.pPixels = pixels.data();
    texInfo.size = RgExtent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    texInfo.filter = RG_SAMPLER_FILTER_LINEAR;

    rgProvideOriginalTexture(GetRTInstance(), &texInfo);
    return name;
}

bool Skybox::Load(const std::string& skyname) {
    if (skyname.empty()) return false;

    static const char* suffixes[6] = { "rt", "lf", "up", "dn", "bk", "ft" };

    bool anyLoaded = false;
    for (int i = 0; i < 6; i++) {
        std::string texName = "sky_" + skyname + suffixes[i];
        std::string path = "nvs1/gfx/env/" + skyname + suffixes[i] + ".tga";
        m_faceTexNames[i] = UploadSkyTexture(path, texName);
        if (!m_faceTexNames[i].empty()) anyLoaded = true;
    }

    if (!anyLoaded) {
        Console::Log("WARNING-> skybox yuklenemedi: " + skyname);
        return false;
    }

    Console::Log("Skybox yuklendi: " + skyname);
    m_loaded = true;
    return true;
}

static void UploadSkyFace(const std::string& texName, const glm::vec3 verts[4], const glm::vec2 uvs[4],
    const glm::vec3& cameraOffset, uint32_t objectID) {
    if (texName.empty()) return;

    RgPrimitiveVertex v[4] = {};
    for (int i = 0; i < 4; i++) {
        v[i].position[0] = verts[i].x + cameraOffset.x;
        v[i].position[1] = verts[i].y + cameraOffset.y;
        v[i].position[2] = verts[i].z + cameraOffset.z;
        v[i].texCoord[0] = uvs[i].x;
        v[i].texCoord[1] = uvs[i].y;
        v[i].color = rgUtilPackColorFloat4D(1.0f, 1.0f, 1.0f, 1.0f);
    }
    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    RgTransform transform{};
    transform.matrix[0][0] = 1.0f; transform.matrix[1][1] = 1.0f; transform.matrix[2][2] = 1.0f;

    RgMeshInfo mesh{};
    mesh.uniqueObjectID = objectID;
    mesh.pMeshName = "skybox";
    mesh.transform = transform;

    RgMeshPrimitiveInfo prim{};
    prim.pPrimitiveNameInMesh = "face";
    prim.pVertices = v;
    prim.vertexCount = 4;
    prim.pIndices = indices;
    prim.indexCount = 6;
    prim.pTextureName = texName.c_str();
    prim.color = rgUtilPackColorFloat4D(1.0f, 1.0f, 1.0f, 1.0f);

    rgUploadMeshPrimitive(GetRTInstance(), &mesh, &prim);
}

void Skybox::Render(const glm::vec3& cameraPos) const {
    if (!m_loaded) return;

    float s = SIZE;
    glm::vec2 uvs[4] = { glm::vec2(0,1), glm::vec2(1,1), glm::vec2(1,0), glm::vec2(0,0) };
    glm::vec2 uvsRot[4] = { glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1) };

    glm::vec3 v0[4] = { {s,-s,-s},{s,-s,s},{s,s,s},{s,s,-s} };
    glm::vec3 v1[4] = { {-s,-s,s},{-s,-s,-s},{-s,s,-s},{-s,s,s} };
    glm::vec3 v2[4] = { {-s,s,s},{s,s,s},{s,s,-s},{-s,s,-s} };
    glm::vec3 v3[4] = { {-s,-s,-s},{s,-s,-s},{s,-s,s},{-s,-s,s} };
    glm::vec3 v4[4] = { {s,-s,s},{-s,-s,s},{-s,s,s},{s,s,s} };
    glm::vec3 v5[4] = { {-s,-s,-s},{s,-s,-s},{s,s,-s},{-s,s,-s} };

    UploadSkyFace(m_faceTexNames[0], v0, uvs, cameraPos, 900001);
    UploadSkyFace(m_faceTexNames[1], v1, uvs, cameraPos, 900002);
    UploadSkyFace(m_faceTexNames[2], v2, uvsRot, cameraPos, 900003);
    UploadSkyFace(m_faceTexNames[3], v3, uvsRot, cameraPos, 900004);
    UploadSkyFace(m_faceTexNames[4], v4, uvs, cameraPos, 900005);
    UploadSkyFace(m_faceTexNames[5], v5, uvs, cameraPos, 900006);
}