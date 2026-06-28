#include "Renderer.h"

#include <glad/glad.h>

bool Renderer::Init() {
    glViewport(0, 0, 1280, 720);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}
void Renderer::BeginFrame()
{
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame()
{
}

void Renderer::Shutdown()
{
}