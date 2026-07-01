#pragma once

#include "Shader.h"
#include "Mesh.h"

class PrimitiveRenderer
{
public:

    static bool Init();
    static void Shutdown();
    static void DrawRect(
        float x,
        float y,
        float width,
        float height,
        float r,
        float g,
        float b,
        float a);

private:
    static Shader s_shader;
    static Mesh s_quad;
    static int s_screenWidth;
    static int s_screenHeight;
};