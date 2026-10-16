#pragma once
#include <cstdint>

// Silah indeksleri hem server.dll hem exe tarafinda ayni degerlerle kullanilir.
enum Weapon {
    PISTOL = 0,
    SHOTGUN = 1
};

// Server'in bir oyuncu icin client'a bildirdigi ozet durum (HUD icin).
struct PlayerStats {
    int32_t health = 0;
    int32_t maxHealth = 0;
    int32_t armor = 0;
    int32_t maxArmor = 0;
    int32_t currentWeapon = PISTOL;
    int32_t bullets = 0;
    int32_t maxBullets = 0;
    int32_t reserveAmmo = 0;
    bool isReloading = false;
    bool alive = true;
};

// Client'in bir tick icinde olusturdugu silah eylemleri. Sayaclar, ayni tick'te
// birden fazla olay olusursa hicbirinin kaybolmamasi icin kullanilir.
struct PlayerActionCommand {
    int32_t attack1Count = 0;
    int32_t attack2Count = 0;
    int32_t reloadCount = 0;
    int32_t switchToWeapon = -1; // -1: degisiklik yok
};