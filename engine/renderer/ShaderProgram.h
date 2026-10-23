#pragma once
#include <string>
#include <unordered_map>
#include <windows.h>
#include <GL/gl.h>

// Tek bir GLSL programi (vertex + fragment). Kaynaklar VFS'ten okunur.
// Uniform ayarlayicilari ilgili program Bind() ile aktifken cagrilmalidir.
// GL kaynaklarini yalnizca Destroy() siler; nesne yok edilirken GL cagrisi
// yapilmaz (statik yok etme sirasinda GL context'i artik olmayabilir).
class ShaderProgram {
public:
    bool Load(const std::string& vertPath, const std::string& fragPath);
    void Destroy();

    bool IsValid() const { return m_program != 0; }
    void Bind() const;
    static void Unbind();

    void SetInt(const char* name, int value);
    void SetFloat(const char* name, float value);
    void SetVec2(const char* name, float x, float y);
    void SetVec3(const char* name, float x, float y, float z);
    void SetVec4(const char* name, float x, float y, float z, float w);
    // count adet vec4 (4 * count float) yukler.
    void SetVec4Array(const char* name, const float* values, int count);

private:
    GLint Location(const char* name);

    GLuint m_program = 0;
    std::unordered_map<std::string, GLint> m_locations;
};