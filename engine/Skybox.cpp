#include "Skybox.h"
#include "TGALoader.h"
#include "console/Console.h"
#include "GLExtensions.h"
#include <vector>

Skybox g_Skybox;

static GLuint UploadSkyTexture(const std::string& path) {
    std::vector<uint8_t> pixels;
    int w = 0, h = 0;
    if (!LoadTGA(path, pixels, w, h)) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void Skybox::Unload() {
    for (int i = 0; i < 6; i++) {
        if (m_faceTex[i] != 0) { glDeleteTextures(1, &m_faceTex[i]); m_faceTex[i] = 0; }
    }
    m_loaded = false;
}

bool Skybox::Load(const std::string& skyname) {
    Unload();
    if (skyname.empty()) return false;

    static const char* suffixes[6] = { "rt", "lf", "up", "dn", "bk", "ft" };

    bool anyLoaded = false;
    for (int i = 0; i < 6; i++) {
        std::string path = "nvs1/gfx/env/" + skyname + suffixes[i] + ".tga";
        m_faceTex[i] = UploadSkyTexture(path);
        if (m_faceTex[i] != 0) anyLoaded = true;
    }

    if (!anyLoaded) {
        Console::Log("WARNING-> skybox yuklenemedi: " + skyname);
        return false;
    }

    Console::Log("Skybox yuklendi: " + skyname);
    m_loaded = true;
    return true;
}

static void DrawSkyFace(GLuint tex, const glm::vec3 verts[4], const glm::vec2 uvs[4]) {
    if (tex == 0) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++) {
        glTexCoord2f(uvs[i].x, uvs[i].y);
        glVertex3f(verts[i].x, verts[i].y, verts[i].z);
    }
    glEnd();
}

void Skybox::Render(const glm::vec3& cameraPos) const {
    if (!m_loaded) return;

    glPushMatrix();
    glTranslatef(cameraPos.x, cameraPos.y, cameraPos.z);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    float s = SIZE;
    glm::vec2 uvs[4] = { glm::vec2(0,1), glm::vec2(1,1), glm::vec2(1,0), glm::vec2(0,0) };
    glm::vec2 uvsRot[4] = { glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1) }; // Bir tık daha (270 derece) döndürülmüş UV

    { glm::vec3 v[4] = { {s,-s,-s},{s,-s,s},{s,s,s},{s,s,-s} };       DrawSkyFace(m_faceTex[0], v, uvs); } // +X rt
    { glm::vec3 v[4] = { {-s,-s,s},{-s,-s,-s},{-s,s,-s},{-s,s,s} };   DrawSkyFace(m_faceTex[1], v, uvs); } // -X lf
    { glm::vec3 v[4] = { {-s,s,s},{s,s,s},{s,s,-s},{-s,s,-s} };       DrawSkyFace(m_faceTex[2], v, uvsRot); } // +Y up (Döndürüldü)
    { glm::vec3 v[4] = { {-s,-s,-s},{s,-s,-s},{s,-s,s},{-s,-s,s} };   DrawSkyFace(m_faceTex[3], v, uvsRot); } // -Y dn (Döndürüldü)
    { glm::vec3 v[4] = { {s,-s,s},{-s,-s,s},{-s,s,s},{s,s,s} };       DrawSkyFace(m_faceTex[4], v, uvs); } // +Z bk
    { glm::vec3 v[4] = { {-s,-s,-s},{s,-s,-s},{s,s,-s},{-s,s,-s} };   DrawSkyFace(m_faceTex[5], v, uvs); } // -Z ft

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}