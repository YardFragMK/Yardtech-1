#pragma once
#include "Weapons.h"
#include "playerstats.h"

// Bir oyuncunun oyun durumu (can, zirh, silahlar). Eski Player sinifinin
// server tarafindaki karsiligi; konsol, kamera veya global durum icermez.
class ServerPlayer {
public:
    int maxHealth = 200;
    int maxArmor = 200;
    int Health = 200;
    int Armor = 200;
    int CurrentWeapon = PISTOL;

    void TakeDamage(int enemyDamage);
    void ApplyActions(const PlayerActionCommand& actions);
    void Update(float deltaTime);
    PlayerStats GetStats() const;

private:
    void Attack(int attackNumber);
    void Reload();
    void SwitchWeapon(int weaponIndex);
    void Death();

    Pistol pistol;
    Shotgun shotgun;
};