#pragma once

#include "Shader.h"
#include "Mesh.h"

class PrimitiveRenderer
{
public:

    static bool Init();
    static void Shutdown();
    static void DrawFullscreen();

private:
    static Shader s_shader;
    static Mesh s_quad;
};