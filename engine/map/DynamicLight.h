#pragma once
#include<glm/glm.hpp>
#include<string>

enum class DynamicLightType {
    Explosion,  // hizli parlayip sonen, kisa omurlu
    Torch,      // surekli hafif titreyen (flicker), kalici
    MuzzleFlash // cok kisa, tek kare gibi anlik parlama
};

struct DynamicLight {
    glm::vec3 position;
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    float radius = 200.0f;       // etkili oldugu mesafe
    float intensity = 1.0f;      // 0-1+ arasi carpan
    DynamicLightType type = DynamicLightType::Torch;

    float age = 0.0f;            // dogumundan bu yana gecen sure (saniye)
    float lifetime = 0.0f;       // 0 = sonsuz (torch gibi), >0 = bu sureden sonra silinir
    bool markedForRemoval = false;

    // Flicker icin (Torch tipi)
    float flickerSeed = 0.0f;    // her isik icin farkli bir baslangic fazi (senkron titremesin diye)
};