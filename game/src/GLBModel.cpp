#include "GLBModel.h"
#include <cstdio>
#include <cstring>

#define CGLTF_IMPLEMENTATION
#include <../extern/cgltf/cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <../extern/stb/stb_image.h>

namespace {
    // image, bir GLB icinde gomulu (buffer_view) ya da harici bir dosyaya
    // (uri) isaret edebilir. Ikisini de tek bir yol uzerinden GL texture'a
    // cevirir.
    GLuint LoadTextureFromCgltfImage(const cgltf_image* image, const std::string& glbDir) {
        if (image == nullptr) return 0;

        int width = 0, height = 0, channels = 0;
        unsigned char* pixels = nullptr;
        bool ownsPixels = false;

        if (image->buffer_view != nullptr) {
            const cgltf_buffer_view* bv = image->buffer_view;
            const unsigned char* data = reinterpret_cast<const unsigned char*>(bv->buffer->data) + bv->offset;
            pixels = stbi_load_from_memory(data, static_cast<int>(bv->size), &width, &height, &channels, 4);
            ownsPixels = true;
        }
        else if (image->uri != nullptr) {
            std::string path = glbDir + image->uri;
            pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
            ownsPixels = true;
        }

        if (pixels == nullptr) {
            printf("GLBModel: texture yuklenemedi (embedded/uri).\n");
            return 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (ownsPixels) stbi_image_free(pixels);
        return tex;
    }
}

bool GLBModel::Load(const std::string& glbPath) {
    m_positions.clear();
    m_normals.clear();
    m_texcoords.clear();
    m_indices.clear();
    if (m_diffuseTexture != 0) {
        glDeleteTextures(1, &m_diffuseTexture);
        m_diffuseTexture = 0;
    }

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
        m_indices.resize(vertexCount);
        for (cgltf_size i = 0; i < vertexCount; i++) {
            m_indices[i] = static_cast<unsigned int>(i);
        }
    }

    // Materyalin base-color (diffuse) texture'ini yukle. Sadece metallic-
    // roughness workflow destekleniyor -- glTF'nin varsayilan/en yaygin
    // materyal modeli bu, "en kolay yol" kapsaminda bu yeterli.
    if (prim.material != nullptr && prim.material->has_pbr_metallic_roughness) {
        const cgltf_texture_view& baseColorView = prim.material->pbr_metallic_roughness.base_color_texture;
        if (baseColorView.texture != nullptr && baseColorView.texture->image != nullptr) {
            size_t slash = glbPath.find_last_of("/\\");
            std::string glbDir = (slash == std::string::npos) ? "" : glbPath.substr(0, slash + 1);
            m_diffuseTexture = LoadTextureFromCgltfImage(baseColorView.texture->image, glbDir);
        }
    }

    cgltf_free(data);

    printf("GLBModel yuklendi: %s (%zu vertex, %zu index, texture=%s)\n",
        glbPath.c_str(), m_positions.size(), m_indices.size(), m_diffuseTexture != 0 ? "var" : "yok");
    return true;
}

void GLBModel::Render() const {
    if (m_positions.empty() || m_indices.empty()) return;

    if (m_diffuseTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_diffuseTexture);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.6f, 0.6f, 0.65f, 1.0f); // texture yoksa duz gri fallback
    }

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

    if (m_diffuseTexture != 0) {
        glDisable(GL_TEXTURE_2D);
    }
}