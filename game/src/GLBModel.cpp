#include "GLBModel.h"
#include <windows.h>
#include <GL/gl.h>
#include <cstdio>

#define CGLTF_IMPLEMENTATION
#include "../../extern/cgltf/cgltf.h"

bool GLBModel::Load(const std::string& glbPath) {
    m_positions.clear();
    m_normals.clear();
    m_texcoords.clear();
    m_indices.clear();

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, glbPath.c_str(), &data);
    if (result != cgltf_result_success) {
        printf("GLBModel: dosya parse edilemedi: %s\n", glbPath.c_str());
        return false;
    }

    result = cgltf_load_buffers(&options, data, glbPath.c_str());
    if (result != cgltf_result_success) {
        printf("GLBModel: buffer'lar yuklenemedi: %s\n", glbPath.c_str());
        cgltf_free(data);
        return false;
    }

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0) {
        printf("GLBModel: dosyada mesh/primitive bulunamadi: %s\n", glbPath.c_str());
        cgltf_free(data);
        return false;
    }

    // Sadece ilk mesh'in ilk primitive'i okunuyor -- "en kolay yol" kapsaminda
    // birden fazla mesh/primitive/node hiyerarsisi desteklenmiyor. Coklu
    // parcali modeller icin ileride genisletilmesi gerekir.
    const cgltf_primitive& prim = data->meshes[0].primitives[0];

    cgltf_accessor* posAccessor = nullptr;
    cgltf_accessor* normalAccessor = nullptr;
    cgltf_accessor* uvAccessor = nullptr;

    for (cgltf_size i = 0; i < prim.attributes_count; i++) {
        const cgltf_attribute& attr = prim.attributes[i];
        if (attr.type == cgltf_attribute_type_position) posAccessor = attr.data;
        else if (attr.type == cgltf_attribute_type_normal) normalAccessor = attr.data;
        else if (attr.type == cgltf_attribute_type_texcoord && uvAccessor == nullptr) uvAccessor = attr.data;
    }

    if (posAccessor == nullptr) {
        printf("GLBModel: POSITION verisi bulunamadi: %s\n", glbPath.c_str());
        cgltf_free(data);
        return false;
    }

    cgltf_size vertexCount = posAccessor->count;
    m_positions.resize(vertexCount);
    for (cgltf_size i = 0; i < vertexCount; i++) {
        float v[3] = { 0.0f, 0.0f, 0.0f };
        cgltf_accessor_read_float(posAccessor, i, v, 3);
        m_positions[i] = glm::vec3(v[0], v[1], v[2]);
    }

    if (normalAccessor != nullptr) {
        m_normals.resize(vertexCount);
        for (cgltf_size i = 0; i < vertexCount; i++) {
            float v[3] = { 0.0f, 1.0f, 0.0f };
            cgltf_accessor_read_float(normalAccessor, i, v, 3);
            m_normals[i] = glm::vec3(v[0], v[1], v[2]);
        }
    }

    if (uvAccessor != nullptr) {
        m_texcoords.resize(vertexCount);
        for (cgltf_size i = 0; i < vertexCount; i++) {
            float v[2] = { 0.0f, 0.0f };
            cgltf_accessor_read_float(uvAccessor, i, v, 2);
            m_texcoords[i] = glm::vec2(v[0], v[1]);
        }
    }

    if (prim.indices != nullptr) {
        cgltf_size indexCount = prim.indices->count;
        m_indices.resize(indexCount);
        for (cgltf_size i = 0; i < indexCount; i++) {
            cgltf_uint idx = 0;
            cgltf_accessor_read_uint(prim.indices, i, &idx, 1);
            m_indices[i] = static_cast<unsigned int>(idx);
        }
    }
    else {
        // Index yoksa, vertex'ler sirali ucgen olarak kabul edilir.
        m_indices.resize(vertexCount);
        for (cgltf_size i = 0; i < vertexCount; i++) {
            m_indices[i] = static_cast<unsigned int>(i);
        }
    }

    cgltf_free(data);

    printf("GLBModel yuklendi: %s (%zu vertex, %zu index)\n",
        glbPath.c_str(), m_positions.size(), m_indices.size());
    return true;
}

void GLBModel::Render() const {
    if (m_positions.empty() || m_indices.empty()) return;

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.6f, 0.6f, 0.65f, 1.0f); // duz gri -- materyal/texture henuz yok

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < m_indices.size(); i++) {
        unsigned int idx = m_indices[i];
        if (idx >= m_positions.size()) continue;

        if (idx < m_normals.size()) {
            glNormal3f(m_normals[idx].x, m_normals[idx].y, m_normals[idx].z);
        }
        if (idx < m_texcoords.size()) {
            glTexCoord2f(m_texcoords[idx].x, m_texcoords[idx].y);
        }
        glVertex3f(m_positions[idx].x, m_positions[idx].y, m_positions[idx].z);
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);
}