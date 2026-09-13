#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Minimal bir glTF/GLB (binary glTF) yukleyici. cgltf kutuphanesini kullanir
// (tek header, MIT lisansli -- sifirdan bir JSON/glTF parser yazmaktan
// cok daha az riskli ve daha az kod). Su asamada SADECE statik geometri
// (pozisyon, normal, UV, index) okunur -- materyal, texture, skin/animasyon
// desteklenmiyor. Model, duz bir renkle (FFP immediate mode) cizilir.
// Ileride texture/materyal eklemek istenirse, bu sinif genisletilebilir.
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
};