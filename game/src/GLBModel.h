#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>

// Minimal bir glTF/GLB yukleyici (cgltf ile). Statik geometri + tek bir
// diffuse texture (base color) okur. Multi-material/multi-primitive/
// animasyon/skin desteklenmiyor -- "en kolay yol" kapsaminda ilk mesh'in
// ilk primitive'i ve onun base-color texture'i kullanilir.
class GLBModel {
public:
    bool Load(const std::string& glbPath);
    void Render() const;
    bool IsLoaded() const { return !m_positions.empty(); }

private:
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texcoords;
    std::vector<unsigned int> m_indices;

    GLuint m_diffuseTexture = 0;
};