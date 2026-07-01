#include "PrimitiveRenderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Shader PrimitiveRenderer::s_shader;
Mesh PrimitiveRenderer::s_quad;

bool PrimitiveRenderer::Init(){
    const char* vertexSource = R"(
#version 330 core
layout(location=0)
in vec3 aPos;
void main(){
    gl_Position = vec4(aPos,1.0);
}
)";

    const char* fragmentSource = R"(
#version 330 core
out vec4 FragColor;
void main(){
    FragColor = vec4(1.0);
}
)";

    if (!s_shader.Load(vertexSource, fragmentSource)) {
        return false;
    }

    if (!s_quad.CreateQuad()) {
        return false;
    }

    return true;
}

void PrimitiveRenderer::Shutdown(){
}

void PrimitiveRenderer::DrawFullscreen(){
    s_shader.Bind();
    s_quad.Draw();
    s_shader.Unbind();
}