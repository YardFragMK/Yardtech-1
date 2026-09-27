#include "RTUI.h"
#include "RTInstance.h"

namespace {
    int s_screenWidth = 1920;
    int s_screenHeight = 1080;

    // Ekran pikseli -> NDC ortografik projeksiyon (glOrtho(0,w,h,0,-1,1) esdegeri).
    // RgDrawFrameInfo::view alaninin "column-major" oldugu belirtildigi icin,
    // aynı sozlesmeyi burada da uyguluyoruz.
    void BuildOrthoMatrix(float outMatrix[16]) {
        float w = static_cast<float>(s_screenWidth);
        float h = static_cast<float>(s_screenHeight);

        for (int i = 0; i < 16; i++) outMatrix[i] = 0.0f;
        outMatrix[0] = 2.0f / w;
        outMatrix[5] = -2.0f / h; // Y ekseni asagi dogru pozitif (ekran uzayi)
        outMatrix[10] = -1.0f;
        outMatrix[12] = -1.0f;
        outMatrix[13] = 1.0f;
        outMatrix[15] = 1.0f;
    }

    void UploadQuad(float x, float y, float w, float h,
        const char* textureName, float u0, float v0, float u1, float v1,
        float r, float g, float b, float a) {

        RgColor4DPacked32 color = rgUtilPackColorFloat4D(r, g, b, a);

        RgPrimitiveVertex verts[4] = {};
        verts[0].position[0] = x;     verts[0].position[1] = y;     verts[0].texCoord[0] = u0; verts[0].texCoord[1] = v0; verts[0].color = color;
        verts[1].position[0] = x + w; verts[1].position[1] = y;     verts[1].texCoord[0] = u1; verts[1].texCoord[1] = v0; verts[1].color = color;
        verts[2].position[0] = x + w; verts[2].position[1] = y + h; verts[2].texCoord[0] = u1; verts[2].texCoord[1] = v1; verts[2].color = color;
        verts[3].position[0] = x;     verts[3].position[1] = y + h; verts[3].texCoord[0] = u0; verts[3].texCoord[1] = v1; verts[3].color = color;

        uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

        RgMeshPrimitiveInfo prim{};
        prim.pPrimitiveNameInMesh = "ui_quad";
        prim.primitiveIndexInMesh = 0;
        prim.flags = (textureName != nullptr) ? 0 : 0;
        prim.pVertices = verts;
        prim.vertexCount = 4;
        prim.pIndices = indices;
        prim.indexCount = 6;
        prim.pTextureName = textureName;
        prim.textureFrame = 0;
        prim.color = color;
        prim.emissive = 0.0f;
        prim.pEditorInfo = nullptr;

        float ortho[16];
        BuildOrthoMatrix(ortho);

        RgViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(s_screenWidth);
        viewport.height = static_cast<float>(s_screenHeight);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        rgUploadNonWorldPrimitive(GetRTInstance(), &prim, ortho, &viewport);
    }
}

void RTUI::SetScreenSize(int width, int height) {
    s_screenWidth = width;
    s_screenHeight = height;
}

void RTUI::FilledRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    UploadQuad(x, y, w, h, nullptr, 0.0f, 0.0f, 1.0f, 1.0f, r, g, b, a);
}

void RTUI::TexturedQuad(float x, float y, float w, float h, const std::string& textureName,
    float u0, float v0, float u1, float v1, float r, float g, float b, float a) {
    UploadQuad(x, y, w, h, textureName.c_str(), u0, v0, u1, v1, r, g, b, a);
}

void RTUI::Line(float x0, float y0, float x1, float y1, float r, float g, float b, float a, float lineWidth) {
    // RTGL1'in ucgen-tabanli primitive API'sinde dogrudan "line" yok --
    // cizgiyi ince bir dikdortgen olarak temsil ediyoruz.
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float nx = -dy / len * (lineWidth * 0.5f);
    float ny = dx / len * (lineWidth * 0.5f);

    RgColor4DPacked32 color = rgUtilPackColorFloat4D(r, g, b, a);

    RgPrimitiveVertex verts[4] = {};
    verts[0].position[0] = x0 + nx; verts[0].position[1] = y0 + ny; verts[0].color = color;
    verts[1].position[0] = x0 - nx; verts[1].position[1] = y0 - ny; verts[1].color = color;
    verts[2].position[0] = x1 - nx; verts[2].position[1] = y1 - ny; verts[2].color = color;
    verts[3].position[0] = x1 + nx; verts[3].position[1] = y1 + ny; verts[3].color = color;

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    RgMeshPrimitiveInfo prim{};
    prim.pPrimitiveNameInMesh = "ui_line";
    prim.pVertices = verts;
    prim.vertexCount = 4;
    prim.pIndices = indices;
    prim.indexCount = 6;
    prim.color = color;

    float ortho[16];
    BuildOrthoMatrix(ortho);

    RgViewport viewport{};
    viewport.width = static_cast<float>(s_screenWidth);
    viewport.height = static_cast<float>(s_screenHeight);
    viewport.maxDepth = 1.0f;

    rgUploadNonWorldPrimitive(GetRTInstance(), &prim, ortho, &viewport);
}

void RTUI::RectOutline(float x, float y, float w, float h, float r, float g, float b, float a, float lineWidth) {
    Line(x, y, x + w, y, r, g, b, a, lineWidth);
    Line(x + w, y, x + w, y + h, r, g, b, a, lineWidth);
    Line(x + w, y + h, x, y + h, r, g, b, a, lineWidth);
    Line(x, y + h, x, y, r, g, b, a, lineWidth);
}