#include"Shader.h"

Shader::Shader() {

}
Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}
