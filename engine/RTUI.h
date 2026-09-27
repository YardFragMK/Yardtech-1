#pragma once
#include <string>

// Ekran-uzayi (2D) UI cizimi icin RTGL1 uzerine kurulu yardimcilar. Eski
// glBegin(GL_QUADS)/glVertex2f desenini rgUploadNonWorldPrimitive'e cevirir.
// Her cagri, o an gecerli pencere boyutuna gore bir ortografik projeksiyon
// matrisi olusturup primitive ile birlikte yukler.

class RTUI {
public:
    static void SetScreenSize(int width, int height);

    static void FilledRect(float x, float y, float w, float h, float r, float g, float b, float a);
    static void RectOutline(float x, float y, float w, float h, float r, float g, float b, float a, float lineWidth = 1.0f);
    static void TexturedQuad(float x, float y, float w, float h, const std::string& textureName,
        float u0, float v0, float u1, float v1, float r, float g, float b, float a);
    static void Line(float x0, float y0, float x1, float y1, float r, float g, float b, float a, float lineWidth = 1.0f);
};