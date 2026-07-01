#include <glad/glad.h>
#include "Renderer.h"



bool Renderer::Init() {
    glViewport(0, 0, 1280, 720);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!PrimitiveRenderer::Init()) {
        return false;
    }

    return true;
}
void Renderer::BeginFrame(){
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    PrimitiveRenderer::DrawRect(
        100.0f,
        100.0f,
        400.0f,
        200.0f,
        0.0f,
        0.0f,
        0.0f,
        0.75f
    );
}

void Renderer::EndFrame(){
}

void Renderer::Shutdown(){
}

