#pragma once
#include <SDL.h>
#include <glm/glm.hpp>

// RayTracedGL1 (RTGL1) tabanli render katmani. Eski Renderer sinifinin
// (OpenGL FFP) yerini bu branch'te alir -- master branch'teki Renderer,
// BSPMapRenderer gibi siniflar bu branch'te kullanilmiyor, RTGL1'in kendi
// upload/draw akisi (rgUploadMeshPrimitive, rgUploadSphericalLight,
// rgDrawFrame) burada yonetiliyor.
class RTRenderer {
public:
    bool Init(SDL_Window* window, int width, int height);
    void Shutdown();

    // Frame'in basinda cagrilir -- RTGL1'e "yeni bir frame basliyor" bilgisini verir.
    void BeginFrame();

    // Frame'in sonunda cagrilir -- toplanan tum geometri/isik verisiyle
    // path-traced goruntuyu urettirip ekrana sunar.
    void EndFrame(const glm::mat4& viewMatrix, float fovYRadians, float nearPlane, float farPlane);

private:
    void* m_instance = nullptr; // RgInstance, header'i cpp'de include ediyoruz
};

extern RTRenderer g_RTRenderer;