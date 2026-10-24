#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// Entity sinif kimlikleri. Engine bu degerleri yorumlamaz; oyun (server.dll) ve onu
// cizen taraf (client/exe) arasinda ortak sozlesmedir.
enum EntityClassId : uint16_t {
    ENT_CLASS_NONE = 0,
    ENT_CLASS_BOT_STILAR = 1,
    ENT_CLASS_PROJECTILE = 2,    // botlarin enerji topu

    // Oyuncu silahlarinin mermileri
    ENT_CLASS_ROCKET = 10,
    ENT_CLASS_GRENADE = 11,
    ENT_CLASS_FOG_BOMB = 12,
    ENT_CLASS_LAVA_BOMB = 13,
    ENT_CLASS_SAWBLADE = 14,
    ENT_CLASS_STAPLE_BALL = 15,
    ENT_CLASS_LAST_BALL = 16,

    // Gecici gorsel etkiler ve alan tehlikeleri
    ENT_CLASS_BEAM = 20,         // raygun isini
    ENT_CLASS_EXPLOSION = 21,
    ENT_CLASS_FOG_CLOUD = 22,
    ENT_CLASS_LAVA_POOL = 23
};

// Gecici etkilerin omru (saniye): yalnizca isin ve patlama icin sabittir; sis ve lav
// havuzunda omur NetEntityState::pitch alaninda tasinir.
constexpr float BEAM_EFFECT_LIFE = 0.30f;
constexpr float EXPLOSION_EFFECT_LIFE = 0.50f;

// Server'in client'a bildirdigi entity ozeti (replikasyon). Alan anlamlari sinifa gore:
//  - mermiler: origin konum, yaw/pitch ucus yonu (derece); testerede flags = donus * 100
//  - BEAM: origin baslangic, yaw/pitch yon, flags = uzunluk, health = yas (santisaniye)
//  - EXPLOSION: flags = yaricap, health = yas (santisaniye)
//  - FOG_CLOUD, LAVA_POOL: flags = yaricap, health = yas (santisaniye), pitch = toplam omur (sn)
struct NetEntityState {
    int32_t id = 0;
    uint16_t classId = ENT_CLASS_NONE;
    uint16_t flags = 0;
    int16_t health = 0;
    glm::vec3 origin{ 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
};