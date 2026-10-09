#include "Skybox.h"
#include "TGALoader.h"
#include "console/Console.h"
#include "GLExtensions.h"
#include "VFSypak.h"
#include <vector>
#include <algorithm>

Skybox g_Skybox;
extern VirtualFileSystem vfs;

static GLuint UploadSkyTextureFromBuffer(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) return 0;

    std::vector<uint8_t> pixels;
    int w = 0, h = 0;

    if (!LoadTGAFromMemory(buffer, pixels, w, h)) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

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
        std::string nameLower = skyname;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

        std::string virtualPath = "gfx/env/" + nameLower + suffixes[i] + ".tga";

        std::vector<uint8_t> fileBuffer = vfs.ReadFile(virtualPath);

        m_faceTex[i] = UploadSkyTextureFromBuffer(fileBuffer);

        if (m_faceTex[i] != 0) {
            anyLoaded = true;
        }
        else {
            std::string fallbackPath = "gfx/env/" + skyname + suffixes[i] + ".tga";
            std::vector<uint8_t> fallbackBuffer = vfs.ReadFile(fallbackPath);
            m_faceTex[i] = UploadSkyTextureFromBuffer(fallbackBuffer);
            if (m_faceTex[i] != 0) anyLoaded = true;
        }
    }

    if (!anyLoaded) {
        Console::Log("WARNING-> skybox yuklenemedi (VFS): " + skyname);
        return false;
    }

    Console::Log("Skybox VFS(.ypak) uzerinden yuklendi: " + skyname);
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
    glTranslated(static_cast<double>(cameraPos.x), static_cast<double>(cameraPos.y), static_cast<double>(cameraPos.z));

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    float s = SIZE;
    float eps = 0.0005f;
    float low = 0.0f + eps;
    float high = 1.0f - eps;

    glm::vec2 uvs[4] = { {low, high}, {high, high}, {high, low}, {low, low} };
    glm::vec2 uvsUp[4] = { {low, low}, {high, low}, {high, high}, {low, high} };
    glm::vec2 uvsDn[4] = { {low, high}, {high, high}, {high, low}, {low, low} };

   
    { glm::vec3 v[4] = { {s,-s,-s}, {s,-s,s}, {s,s,s}, {s,s,-s} };       DrawSkyFace(m_faceTex[0], v, uvs); } // Right (+X) -> rt
    { glm::vec3 v[4] = { {-s,-s,s}, {-s,-s,-s}, {-s,s,-s}, {-s,s,s} };   DrawSkyFace(m_faceTex[1], v, uvs); } // Left (-X) -> lf
    { glm::vec3 v[4] = { {-s,s,s}, {s,s,s}, {s,s,-s}, {-s,s,-s} };       DrawSkyFace(m_faceTex[2], v, uvsUp); } // Up (+Y) -> up 
    { glm::vec3 v[4] = { {-s,-s,-s}, {s,-s,-s}, {s,-s,s}, {-s,-s,s} };   DrawSkyFace(m_faceTex[3], v, uvsDn); } // Down (-Y) -> dn 
    { glm::vec3 v[4] = { {s,-s,s}, {-s,-s,s}, {-s,s,s}, {s,s,s} };       DrawSkyFace(m_faceTex[4], v, uvs); } // Back (+Z) -> bk
    { glm::vec3 v[4] = { {-s,-s,-s}, {s,-s,-s}, {s,s,-s}, {-s,s,-s} };   DrawSkyFace(m_faceTex[5], v, uvs); } // Front (-Z) -> ft

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}