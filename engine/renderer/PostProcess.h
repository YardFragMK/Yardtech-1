#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>
#include "viewsetup.h"

class ShaderProgram;

// Sahne cizildikten sonra, arayuzden once calisan tam ekran efekt zinciri:
// su alti -> kara delik -> ruya -> CRT. FBO kullanmaz; her gecis ekrani bir
// dokuya kopyalayip shader ile yeniden cizer. Efektler cvar'larla kontrol edilir
// ve ilgili shader dosyalari bulunamazsa sessizce atlanir.
class PostProcess {
public:
    bool Init(int width, int height);
    void Apply(const glm::dmat4& view, const glm::dmat4& projection, const ViewSetup& viewSetup);

    // Oyuncunun su icinde olup olmadigini bildirir (otomatik su alti efekti).
    // r_underwater cvar'i ise elle zorlamadir; ikisinden buyugu kullanilir.
    void SetUnderwater(bool underwater) { m_underwaterTarget = underwater; }

private:
    struct BlackHole {
        glm::vec3 position{ 0.0f };
        float radius = 64.0f;   // olay ufku yaricapi (dunya birimi)
        float strength = 1.0f;
        float fade = 0.0f;      // gorus hatti acikken 1, engelliyken 0'a yumusakca iner
    };

    void CreateTexture(GLuint& texture);
    void CopyFramebufferTo(GLuint texture);
    void RefreshBlackHoles();
    int BuildHoleUniforms(const glm::dmat4& view, const glm::dmat4& projection,
        const ViewSetup& viewSetup, float deltaTime, float outData[16]);
    void BeginPass(ShaderProgram& program);
    void EndPass();

    bool m_ready = false;
    int m_width = 0;
    int m_height = 0;
    GLuint m_sceneTexture = 0;
    GLuint m_historyTexture = 0; // ruya efektinin onceki kare dokusu (iz/hayalet etkisi)

    int* m_masterCvar = nullptr;
    float* m_crtCvar = nullptr;
    float* m_dreamCvar = nullptr;
    float* m_underwaterCvar = nullptr;
    int* m_blackHoleCvar = nullptr;
    int* m_blackHoleTestCvar = nullptr;

    bool m_underwaterTarget = false;
    float m_underwaterAuto = 0.0f;
    bool m_dreamWasActive = false;

    float m_time = 0.0f;
    uint32_t m_lastTicks = 0;

    std::string m_holesLevel;
    std::vector<BlackHole> m_blackHoles;
    BlackHole m_testHole;
    bool m_testHoleActive = false;
};