#pragma once
#include <glm/glm.hpp>
#include "viewsetup.h"

// client.dll'in sahip oldugu oyuncu kamerasi: pozisyon, bakis acilari, goz
// yuksekligi, roll, yurume sallanmasi (view bob) ve inis sarsintisi. Ayarlar
// (hassasiyet, roll siniri vb.) cvar'lardan okunup metotlara parametre olarak
// verilir; bu sinif engine'e veya global duruma bagli degildir.
class ClientCamera {
public:
    glm::vec3 position{ 0.0f, 60.0f, 0.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float targetRoll = 0.0f;
    bool isCrouching = false;

    float eyeHeightOffset = 30.0f;
    float eyeHeightStanding = 28.0f;
    float eyeHeightCrouching = 12.0f;
    float eyeHeightLerpSpeed = 10.0f;

    float bobTimer = 0.0f;
    float bobOffsetY = 0.0f;
    float bobOffsetX = 0.0f;
    float doomBobPhase = 0.0f;

    // Yere sert inis anindaki dikey kamera darbesi: inis aninda negatif bir
    // deger atanir, sonra sifira dogru soner.
    float landingShakeOffset = 0.0f;
    float landingShakeDecaySpeed = 9.0f;

    glm::vec3 Forward() const;
    glm::vec3 Right() const;
    glm::vec3 GetEyePosition() const;

    void Reset(const glm::vec3& newPosition, float newYaw);
    void ProcessMouseMovement(float xOffset, float yOffset, float sensitivity);

    // Yanal harekete gore kamerayi saga/sola yatirma hedefini belirler.
    void SetRollInput(float moveRightAxis, float maxRoll);

    // Her sabit tick'te bir kez cagrilir.
    void Update(float deltaTime, float rollLerpSpeed);
    void UpdateViewBob(float deltaTime, float horizontalSpeed, bool grounded, int style);
    void TriggerLandingShake(float shakeAmount);

    void FillView(ViewSetup* outView) const;
};