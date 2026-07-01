#include<iostream>
#include"Shader.h"
#include <glm/gtc/type_ptr.hpp>

Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }

}
bool Shader::Load(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    if (!CheckShader(vertexShader)) {
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource( fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    if (!CheckShader(fragmentShader)) {
        glDeleteShader(fragmentShader);
        return false;
    }

    m_program = glCreateProgram();

    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);

    glLinkProgram(m_program);

    if (!CheckProgram(m_program)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_program);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void Shader::Bind() const {
    glUseProgram(m_program);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

GLuint Shader::GetProgram() const {
    return m_program;
}

bool Shader::CheckShader(GLuint shader) {
    GLint success;

    glGetShaderiv( shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog( shader, 512, nullptr, infoLog);
        std::cout<< "[SHADER ERROR]\n"<< infoLog<< std::endl;
        return false;
    }
    return true;
}

bool Shader::CheckProgram(GLuint program) {
    GLint success;

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog( program, 512, nullptr, infoLog);
        std::cout<< "[PROGRAM ERROR]\n" << infoLog << std::endl;
        return false;
    }
    return true;
}
GLint Shader::GetUniformLocation(
    const std::string& name)
{
    return glGetUniformLocation(
        m_program,
        name.c_str());
}

void Shader::SetInt(
    const std::string& name,
    int value)
{
    glUniform1i(
        GetUniformLocation(name),
        value);
}

void Shader::SetFloat(
    const std::string& name,
    float value)
{
    glUniform1f(
        GetUniformLocation(name),
        value);
}

void Shader::SetVec2(
    const std::string& name,
    const glm::vec2& value)
{
    glUniform2fv(
        GetUniformLocation(name),
        1,
        glm::value_ptr(value));
}

void Shader::SetVec3(
    const std::string& name,
    const glm::vec3& value)
{
    glUniform3fv(
        GetUniformLocation(name),
        1,
        glm::value_ptr(value));
}

void Shader::SetVec4(
    const std::string& name,
    const glm::vec4& value)
{
    glUniform4fv(
        GetUniformLocation(name),
        1,
        glm::value_ptr(value));
}

void Shader::SetMat4(
    const std::string& name,
    const glm::mat4& value)
{
    glUniformMatrix4fv(
        GetUniformLocation(name),
        1,
        GL_FALSE,
        glm::value_ptr(value));
}
