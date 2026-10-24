#pragma once
#include <glm/glm.hpp>
#include "playerstats.h"

// Bir oyuncunun oyun durumu: can, zirh ve 10 silahin cephanesi, zamanlayicilari ve
// ates mantigi. Konsol, kamera veya global durum icermez; girdi QueueActions ile
// biriktirilir, her sabit tick'te Think ile islenir.
class ServerPlayer {
public:
    ServerPlayer();

    int maxHealth = 200;
    int maxArmor = 200;
    int Health = 200;
    int Armor = 200;
    int CurrentWeapon = PISTOL;

    void TakeDamage(int enemyDamage);

    // Client'tan gelen girdiyi biriktirir (sayaclar toplanir, tutma ve nisan son degerini alir).
    void QueueActions(const PlayerActionCommand& actions);

    // Bir tick'lik silah mantigi: degistirme, dolum, ates, donus/sarj, FOV.
    void Think(float deltaTime, int ownerIndex, const glm::vec3& eye);

    PlayerStats GetStats() const;

private:
    struct WeaponAmmo {
        int clip = 0;
        int reserve = 0;
    };

    void StartReload();
    void UpdateReload(float deltaTime);
    void CancelActivities();
    void UpdateWeapon(float deltaTime, int owner, const glm::vec3& eye,
        const glm::vec3& forward, const glm::vec3& right, const glm::vec3& up);
    void Death();

    WeaponAmmo m_ammo[WEAPON_COUNT];
    PlayerActionCommand m_input;

    float m_cooldown = 0.0f;
    bool m_reloading = false;
    float m_reloadTimer = 0.0f;

    float m_spin = 0.0f;        // testere donusu 0..1
    bool m_charging = false;    // lastgun sarj ediyor
    float m_charge = 0.0f;      // lastgun sarji 0..1
    float m_fovBoost = 0.0f;    // derece
};