#include "GLBModel.h"
#include "VFSypak.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CGLTF_IMPLEMENTATION
#include <../extern/cgltf/cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <../extern/stb/stb_image.h>

extern VirtualFileSystem vfs;

namespace {

    // cgltf'in harici dosyalari (.bin, texture) diskten degil VFS'ten okumasi icin geri cagrilar.
    // Bellek malloc ile ayrilir; cgltf varsayilan olarak free ile birakir.
    cgltf_result ReadFileFromVfs(const cgltf_memory_options* memoryOptions, const cgltf_file_options* fileOptions,
        const char* path, cgltf_size* size, void** data) {
        (void)fileOptions;

        const std::vector<uint8_t> file = vfs.ReadFile(path);
        if (file.empty()) return cgltf_result_file_not_found;

        void* memory = memoryOptions->alloc_func
            ? memoryOptions->alloc_func(memoryOptions->user_data, file.size())
            : std::malloc(file.size());
        if (memory == nullptr) return cgltf_result_out_of_memory;

        std::memcpy(memory, file.data(), file.size());
        if (size != nullptr) *size = file.size();
        if (data != nullptr) *data = memory;
        return cgltf_result_success;
    }


    void ReleaseFileFromVfs(
        const cgltf_memory_options* memoryOptions,
        const cgltf_file_options* fileOptions,
        void* data,
        cgltf_size size)
    {
        (void)fileOptions;
        (void)size;

        if (memoryOptions->free_func)
            memoryOptions->free_func(memoryOptions->user_data, data);
        else
            std::free(data);
    }


    // Bir GLB icindeki gomulu (buffer_view) ya da harici (uri) gorseli GL texture'a cevirir.
    GLuint LoadTextureFromCgltfImage(const cgltf_image* image, const std::string& glbDirectory) {
        if (image == nullptr) return 0;

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pixels = nullptr;

        if (image->buffer_view != nullptr) {
            // Gomulu gorsel: GLB'nin kendi verisinden okunur.
            const cgltf_buffer_view* view = image->buffer_view;
            const unsigned char* data = reinterpret_cast<const unsigned char*>(view->buffer->data) + view->offset;
            pixels = stbi_load_from_memory(data, static_cast<int>(view->size), &width, &height, &channels, 4);
        }
        else if (image->uri != nullptr) {
            if (std::strncmp(image->uri, "data:", 5) == 0) {
                printf("GLBModel: 'data:' uri'li gorseller desteklenmiyor.\n");
                return 0;
            }

            // Dosya adindaki "%20" gibi kodlamalar cozulur, sonra VFS'ten okunur.
            std::string uri = image->uri;
            cgltf_decode_uri(&uri[0]);
            uri.resize(std::strlen(uri.c_str()));

            const std::vector<uint8_t> file = vfs.ReadFile(glbDirectory + uri);
            if (!file.empty()) {
                pixels = stbi_load_from_memory(file.data(), static_cast<int>(file.size()), &width, &height, &channels, 4);
            }
        }

        if (pixels == nullptr) {
            printf("GLBModel: texture yuklenemedi (gomulu/uri).\n");
            return 0;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(pixels);
        return texture;
    }

}  // namespace

bool GLBModel::Load(const std::string& glbPath) {
    m_positions.clear();
    m_normals.clear();
    m_texcoords.clear();
    m_indices.clear();
    if (m_diffuseTexture != 0) {
        glDeleteTextures(1, &m_diffuseTexture);
        m_diffuseTexture = 0;
    }

    // Dosya VFS'ten (paket ya da klasor) okunur. GLB'nin gomulu verisi bu bellege isaret eder;
    // bu yuzden 'file' fonksiyon sonuna kadar yasamalidir.
    const std::vector<uint8_t> file = vfs.ReadFile(glbPath);
    if (file.empty()) {
        printf("GLBModel: dosya VFS icinde bulunamadi: %s\n", glbPath.c_str());
        return false;
    }

    cgltf_options options = {};
    options.file.read = ReadFileFromVfs;
    options.file.release = ReleaseFileFromVfs;
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse(&options, file.data(), file.size(), &data);
    if (result != cgltf_result_success) {
        printf("GLBModel: dosya parse edilemedi: %s\n", glbPath.c_str());
        return false;
    }

    // Harici .bin dosyalari icin glTF'nin yolu verilir; geri cagri bunlari VFS'ten okur.
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

    const cgltf_primitive& primitive = data->meshes[0].primitives[0];

    cgltf_accessor* positionAccessor = nullptr;
    cgltf_accessor* normalAccessor = nullptr;
    cgltf_accessor* uvAccessor = nullptr;

    for (cgltf_size i = 0; i < primitive.attributes_count; i++) {
        const cgltf_attribute& attribute = primitive.attributes[i];
        if (attribute.type == cgltf_attribute_type_position) positionAccessor = attribute.data;
        else if (attribute.type == cgltf_attribute_type_normal) normalAccessor = attribute.data;
        else if (attribute.type == cgltf_attribute_type_texcoord && uvAccessor == nullptr) uvAccessor = attribute.data;
    }

    if (positionAccessor == nullptr) {
        printf("GLBModel: POSITION verisi bulunamadi: %s\n", glbPath.c_str());
        cgltf_free(data);
        return false;
    }

    const cgltf_size vertexCount = positionAccessor->count;
    m_positions.resize(vertexCount);
    for (cgltf_size i = 0; i < vertexCount; i++) {
        float v[3] = { 0.0f, 0.0f, 0.0f };
        cgltf_accessor_read_float(positionAccessor, i, v, 3);
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

    if (primitive.indices != nullptr) {
        const cgltf_size indexCount = primitive.indices->count;
        m_indices.resize(indexCount);
        for (cgltf_size i = 0; i < indexCount; i++) {
            cgltf_uint index = 0;
            cgltf_accessor_read_uint(primitive.indices, i, &index, 1);
            m_indices[i] = static_cast<unsigned int>(index);
        }
    }
    else {
        // Indeks listesi yoksa kose noktalari sirayla ucgen olusturur.
        m_indices.resize(vertexCount);
        for (cgltf_size i = 0; i < vertexCount; i++) {
            m_indices[i] = static_cast<unsigned int>(i);
        }
    }

    // Materyalin base-color (diffuse) texture'i yuklenir. Yalnizca metallic-roughness
    // is akisi desteklenir (glTF'nin varsayilan ve en yaygin modeli).
    if (primitive.material != nullptr && primitive.material->has_pbr_metallic_roughness) {
        const cgltf_texture_view& baseColorView = primitive.material->pbr_metallic_roughness.base_color_texture;
        if (baseColorView.texture != nullptr && baseColorView.texture->image != nullptr) {
            const size_t slash = glbPath.find_last_of("/\\");
            const std::string glbDirectory = (slash == std::string::npos) ? "" : glbPath.substr(0, slash + 1);
            m_diffuseTexture = LoadTextureFromCgltfImage(baseColorView.texture->image, glbDirectory);
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
        glColor4f(0.6f, 0.6f, 0.65f, 1.0f);   // texture yoksa duz gri
    }

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < m_indices.size(); i++) {
        const unsigned int index = m_indices[i];
        if (index >= m_positions.size()) continue;

        if (index < m_normals.size()) {
            glNormal3f(m_normals[index].x, m_normals[index].y, m_normals[index].z);
        }
        if (index < m_texcoords.size()) {
            glTexCoord2f(m_texcoords[index].x, m_texcoords[index].y);
        }
        glVertex3f(m_positions[index].x, m_positions[index].y, m_positions[index].z);
    }
    glEnd();

    if (m_diffuseTexture != 0) {
        glDisable(GL_TEXTURE_2D);
    }
}