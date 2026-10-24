#pragma once
#include <cstdint>

// Silah indeksleri; client (klavye 1..9, 0), server (silah mantigi) ve HUD ayni degerleri kullanir.
enum Weapon {
    PISTOL = 0,
    DEAGLE,
    SHOTGUN,
    DOUBLE_BARREL,
    SAWBLADE,
    GRENADE_LAUNCHER,
    ROCKET_LAUNCHER,
    STAPLEGUN,
    RAYGUN,
    LASTGUN,
    WEAPON_COUNT
};

// Server'in bir oyuncu icin client'a bildirdigi ozet durum (HUD ve kamera icin).
struct PlayerStats {
    int32_t health = 0;
    int32_t maxHealth = 0;
    int32_t armor = 0;
    int32_t maxArmor = 0;
    int32_t currentWeapon = PISTOL;
    int32_t bullets = 0;      // sarjorlu silahta sarjor, sarjorsuzda toplam mermi
    int32_t maxBullets = 0;
    int32_t reserveAmmo = 0;  // sarjorsuz silahlarda 0
    bool isReloading = false;
    bool alive = true;
    float charge = 0.0f;      // 0..1: testere donusu ya da lastgun sarji
    float fovBoost = 0.0f;    // derece: lastgun sarjinda gorus acisina eklenir
};

// Client'in bir tick icinde olusturdugu silah girdisi. Sayaclar tus BASILDIGI anlari
// (ayni tick'te birden fazla olay kaybolmasin diye), "held" alanlari tusun SU AN basili
// olup olmadigini tasir (donus/sarj silahlari icin). Nisan acilari ates yonunu belirler.
struct PlayerActionCommand {
    int32_t attack1Count = 0;
    int32_t attack2Count = 0;
    int32_t reloadCount = 0;
    int32_t switchToWeapon = -1; // -1: degisiklik yok
    bool attack1Held = false;
    bool attack2Held = false;
    float aimYaw = 0.0f;
    float aimPitch = 0.0f;
};