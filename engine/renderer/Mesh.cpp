#include"Mesh.h"
Mesh::Mesh() {

}
Mesh::~Mesh() {
    Destroy();
}

bool Mesh::CreateQuad() {
    float vertices[] = {
        0.0f,0.0f,0.0f,
        1.0f,0.0f,0.0f,
        1.0f,1.0f,0.0f,

        0.0f,0.0f,0.0f,
        1.0f,1.0f,0.0f,
        0.0f,1.0f,0.0f
    };
    m_vertexCount = 6;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return true;
}

void Mesh::Draw() const
{
    glBindVertexArray(m_vao);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        m_vertexCount);

    glBindVertexArray(0);
}

void Mesh::Destroy()
{
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}