#include "PrimitiveRenderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Shader PrimitiveRenderer::s_shader;
Mesh PrimitiveRenderer::s_quad;

glm::mat4 PrimitiveRenderer::s_projection;

int PrimitiveRenderer::s_screenWidth = 1280;
int PrimitiveRenderer::s_screenHeight = 720;


bool PrimitiveRenderer::Init(){
    const char* vertexSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uProjection;
uniform mat4 uModel;

void main()
{
    gl_Position =
        uProjection *
        uModel *
        vec4(aPos,1.0);
}
)";

    const char* fragmentSource = R"(
#version 330 core

out vec4 FragColor;

uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
)";

    if (!s_shader.Load(vertexSource, fragmentSource)) {
        return false;
    }

    if (!s_quad.CreateQuad()) {
        return false;
    }
    s_projection = glm::ortho(
        0.0f,
        (float)s_screenWidth,
        (float)s_screenHeight,
        0.0f
    );
    return true;
}

void PrimitiveRenderer::Shutdown(){
}
void PrimitiveRenderer::DrawRect(
    float x,
    float y,
    float width,
    float height,
    float r,
    float g,
    float b,
    float a)
{
    glm::mat4 model(1.0f);

    model = glm::translate(
        model,
        glm::vec3(x, y, 0.0f));

    model = glm::scale(
        model,
        glm::vec3(width, height, 1.0f));

    s_shader.Bind();

    s_shader.SetMat4(
        "uProjection",
        s_projection);

    s_shader.SetMat4(
        "uModel",
        model);

    s_shader.SetVec4(
        "uColor",
        glm::vec4(r, g, b, a));

    s_quad.Draw();

    s_shader.Unbind();
}
