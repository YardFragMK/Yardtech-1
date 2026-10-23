#include "ShaderProgram.h"
#include "../GLExtensions.h"
#include "../console/Console.h"
#include "VFSypak.h"
#include <sstream>
#include <vector>

extern VirtualFileSystem vfs;

namespace {
    std::string ReadText(const std::string& path) {
        const std::vector<uint8_t> data = vfs.ReadFile(path);
        return std::string(data.begin(), data.end());
    }

    // Derleyici gunlugunu satir satir konsola yazar (en fazla 12 satir).
    void LogLines(const std::string& prefix, const std::string& text) {
        std::istringstream stream(text);
        std::string line;
        int count = 0;

        while (count < 12 && std::getline(stream, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\0')) line.pop_back();
            if (line.empty()) continue;

            Console::Log(prefix + line);
            ++count;
        }
    }

    GLuint CompileStage(GLenum type, const std::string& source, const std::string& label) {
        const GLuint shader = glCreateShader_(type);
        const char* text = source.c_str();
        const GLint length = static_cast<GLint>(source.size());
        glShaderSource_(shader, 1, &text, &length);
        glCompileShader_(shader);

        GLint status = 0;
        glGetShaderiv_(shader, GL_COMPILE_STATUS, &status);
        if (status == 0) {
            GLint logLength = 0;
            glGetShaderiv_(shader, GL_INFO_LOG_LENGTH, &logLength);

            std::string log(static_cast<size_t>(logLength > 1 ? logLength : 1), '\0');
            glGetShaderInfoLog_(shader, static_cast<GLsizei>(log.size()), nullptr, &log[0]);

            Console::Log("Shader derlenemedi: " + label);
            LogLines("  ", log);
            glDeleteShader_(shader);
            return 0;
        }
        return shader;
    }
}

bool ShaderProgram::Load(const std::string& vertPath, const std::string& fragPath) {
    Destroy();
    if (!GLShadersAvailable()) return false;

    const std::string vertSource = ReadText(vertPath);
    const std::string fragSource = ReadText(fragPath);
    if (vertSource.empty() || fragSource.empty()) {
        Console::Log("Shader dosyasi bulunamadi: " + (vertSource.empty() ? vertPath : fragPath));
        return false;
    }

    const GLuint vertex = CompileStage(GL_VERTEX_SHADER, vertSource, vertPath);
    const GLuint fragment = CompileStage(GL_FRAGMENT_SHADER, fragSource, fragPath);
    if (vertex == 0 || fragment == 0) {
        if (vertex != 0) glDeleteShader_(vertex);
        if (fragment != 0) glDeleteShader_(fragment);
        return false;
    }

    const GLuint program = glCreateProgram_();
    glAttachShader_(program, vertex);
    glAttachShader_(program, fragment);
    glLinkProgram_(program);

    // Program baglandiktan sonra tekil shader nesneleri silinebilir.
    glDeleteShader_(vertex);
    glDeleteShader_(fragment);

    GLint status = 0;
    glGetProgramiv_(program, GL_LINK_STATUS, &status);
    if (status == 0) {
        GLint logLength = 0;
        glGetProgramiv_(program, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<size_t>(logLength > 1 ? logLength : 1), '\0');
        glGetProgramInfoLog_(program, static_cast<GLsizei>(log.size()), nullptr, &log[0]);

        Console::Log("Shader baglanamadi: " + fragPath);
        LogLines("  ", log);
        glDeleteProgram_(program);
        return false;
    }

    m_program = program;
    return true;
}

void ShaderProgram::Destroy() {
    if (m_program != 0 && glDeleteProgram_ != nullptr) {
        glDeleteProgram_(m_program);
    }
    m_program = 0;
    m_locations.clear();
}

void ShaderProgram::Bind() const {
    if (m_program != 0 && glUseProgram_ != nullptr) glUseProgram_(m_program);
}

void ShaderProgram::Unbind() {
    if (glUseProgram_ != nullptr) glUseProgram_(0);
}

GLint ShaderProgram::Location(const char* name) {
    auto it = m_locations.find(name);
    if (it != m_locations.end()) return it->second;

    GLint location = glGetUniformLocation_(m_program, name);
    if (location < 0) {
        // Bazi suruculer dizi uniform'larini yalnizca "ad[0]" ile bulur.
        const std::string arrayName = std::string(name) + "[0]";
        location = glGetUniformLocation_(m_program, arrayName.c_str());
    }
    m_locations.emplace(name, location);
    return location; // -1: shader'da yok ya da kullanilmadigi icin elenmis; ayarlama yok sayilir
}

void ShaderProgram::SetInt(const char* name, int value) {
    if (m_program == 0) return;
    const GLint location = Location(name);
    if (location >= 0) glUniform1i_(location, value);
}

void ShaderProgram::SetFloat(const char* name, float value) {
    if (m_program == 0) return;
    const GLint location = Location(name);
    if (location >= 0) glUniform1f_(location, value);
}

void ShaderProgram::SetVec2(const char* name, float x, float y) {
    if (m_program == 0) return;
    const GLint location = Location(name);
    if (location >= 0) glUniform2f_(location, x, y);
}

void ShaderProgram::SetVec3(const char* name, float x, float y, float z) {
    if (m_program == 0) return;
    const GLint location = Location(name);
    if (location >= 0) glUniform3f_(location, x, y, z);
}

void ShaderProgram::SetVec4(const char* name, float x, float y, float z, float w) {
    if (m_program == 0) return;
    const GLint location = Location(name);
    if (location >= 0) glUniform4f_(location, x, y, z, w);
}

void ShaderProgram::SetVec4Array(const char* name, const float* values, int count) {
    if (m_program == 0 || values == nullptr) return;
    const GLint location = Location(name);
    if (location >= 0) glUniform4fv_(location, count, values);
}