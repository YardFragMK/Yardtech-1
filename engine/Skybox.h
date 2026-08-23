#pragma once
#include <string>
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>

class Skybox {
public:
    // skyname: worldspawn "skyname" key'i (orn. "desert"). gfx/env/<skyname><suffix>.tga arar.
    bool Load(const std::string& skyname);
    void Unload();
    void Render(const glm::vec3& cameraPos) const;
    bool IsLoaded() const { return m_loaded; }

private:
    // sira: +X(rt) -X(lf) +Y(up) -Y(dn) +Z(bk) -Z(ft) -- engine (Y-up) eksenlerine gore
    GLuint m_faceTex[6] = { 0,0,0,0,0,0 };
    bool m_loaded = false;
    static constexpr float SIZE = 4000.0f;
};

extern Skybox g_Skybox;