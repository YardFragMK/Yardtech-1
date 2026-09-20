#define RG_USE_SURFACE_WIN32
#include "RTRenderer.h"
#include <RTGL1.h>
#include <SDL_syswm.h>
#include <cstdio>

RTRenderer g_RTRenderer;

bool RTRenderer::Init(SDL_Window* window, int width, int height) {
    // SDL penceresinden Win32 HWND/HINSTANCE'i cikar -- RTGL1'in Vulkan
    // surface'i bunlarla olusturuluyor.
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        printf("RTRenderer: SDL WM bilgisi alinamadi: %s\n", SDL_GetError());
        return false;
    }

    RgWin32SurfaceCreateInfo win32Surface{};
    win32Surface.hinstance = GetModuleHandle(nullptr);
    win32Surface.hwnd = wmInfo.info.win.window;

    RgInstanceCreateInfo createInfo{};
    createInfo.pAppName = "Yardtech-1 RT";
    createInfo.pAppGUID = "b8f1a2e4-yardtech1-raytracing-branch";
    createInfo.pWin32SurfaceInfo = &win32Surface;

    createInfo.pConfigPath = "RayTracedGL1.txt";
    createInfo.pOverrideFolderPath = "resources";

    // Rasterize edilen (immediate-mode benzeri) geometri icin ayrilan bellek
    // -- HUD/Console gibi 2D overlay'ler bu yoldan cizilecek, dunya
    // geometrisi ise tam path-traced (rgUploadMeshPrimitive) olacak.
    createInfo.rasterizedMaxVertexCount = 1u << 16;
    createInfo.rasterizedMaxIndexCount = 1u << 17;
    createInfo.rasterizedVertexColorGamma = RG_TRUE;

    createInfo.rasterizedSkyCubemapSize = 256;

    createInfo.primaryRaysMaxAlbedoLayers = 1;
    createInfo.indirectIlluminationMaxAlbedoLayers = 1;

    createInfo.rayCullBackFacingTriangles = RG_TRUE;

    // Motorun mevcut koordinat sistemi: Y-up, sag-el -- BSPMap/Camera'nin
    // zaten kullandigi eksen kuraliyla ayni, ek bir donusume gerek yok.
    createInfo.worldUp = RgFloat3D{ { 0.0f, 1.0f, 0.0f } };
    createInfo.worldForward = RgFloat3D{ { 0.0f, 0.0f, 1.0f } };
    createInfo.worldScale = 0.03f; // GoldSrc birimleri (~inch) -> metre yaklasik olcek, ince ayar gerekebilir

    RgInstance instance = RG_NULL_HANDLE;
    RgResult result = rgCreateInstance(&createInfo, &instance);

    if (result != RG_RESULT_SUCCESS) {
        printf("RTRenderer: rgCreateInstance basarisiz, kod: %d\n", static_cast<int>(result));
        return false;
    }

    m_instance = instance;
    printf("RTRenderer: RTGL1 instance basariyla olusturuldu.\n");
    return true;
}

void RTRenderer::Shutdown() {
    if (m_instance != nullptr) {
        rgDestroyInstance(static_cast<RgInstance>(m_instance));
        m_instance = nullptr;
    }
}

void RTRenderer::BeginFrame() {
    if (m_instance == nullptr) return;

    RgStartFrameInfo startInfo{};
    startInfo.pMapName = "yardtech1";
    startInfo.ignoreExternalGeometry = RG_FALSE;

    rgStartFrame(static_cast<RgInstance>(m_instance), &startInfo);
}

void RTRenderer::EndFrame(const glm::mat4& viewMatrix, float fovYRadians, float nearPlane, float farPlane) {
    if (m_instance == nullptr) return;

    RgDrawFrameInfo drawInfo{};
    // RgDrawFrameInfo::view column-major 4x4 bekliyor -- glm zaten
    // column-major depoluyor, dogrudan kopyalanabilir.
    const float* viewPtr = &viewMatrix[0][0];
    for (int i = 0; i < 16; i++) {
        drawInfo.view[i] = viewPtr[i];
    }

    drawInfo.fovYRadians = fovYRadians;
    drawInfo.cameraNear = nearPlane;
    drawInfo.cameraFar = farPlane;
    drawInfo.rayLength = 10000.0f;
    drawInfo.rayCullMaskWorld = RG_DRAW_FRAME_RAY_CULL_WORLD_0_BIT | RG_DRAW_FRAME_RAY_CULL_SKY_BIT;
    drawInfo.currentTime = static_cast<double>(SDL_GetTicks()) / 1000.0;
    drawInfo.vsync = RG_TRUE;
    drawInfo.pParams = nullptr; // ileride tonemapping/bloom gibi parametreleri buraya zincirleyecegiz

    rgDrawFrame(static_cast<RgInstance>(m_instance), &drawInfo);
}