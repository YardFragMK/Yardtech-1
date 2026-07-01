#include<iostream>
#include"Shader.h"

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
