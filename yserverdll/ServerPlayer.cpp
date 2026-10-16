#include "pch.h"
#include "ServerPlayer.h"

void ServerPlayer::TakeDamage(int enemyDamage) {
    if (Armor > 0) {
        int armorDamage = enemyDamage * 70 / 100;
        int healthDamage = enemyDamage * 30 / 100;

        if (Armor >= armorDamage) {
            Armor -= armorDamage;
            Health -= healthDamage;
        }
        else {
            int remaining = armorDamage - Armor;
            Armor = 0;
            Health -= healthDamage + remaining;
        }
    }
    else {
        Health -= enemyDamage;
    }

    if (Health < 0) {
        Health = 0;
        Death();
    }
}

void ServerPlayer::Attack(int attackNumber) {
    if (attackNumber == 1) {
        if (CurrentWeapon == PISTOL) pistol.Shoot(1);
        else if (CurrentWeapon == SHOTGUN) shotgun.Shoot(1);
    }
    else if (attackNumber == 2) {
        if (CurrentWeapon == SHOTGUN) shotgun.Shoot(2);
    }
}

void ServerPlayer::Reload() {
    if (CurrentWeapon == PISTOL) pistol.Reload();
    else shotgun.StartReload();
}

void ServerPlayer::SwitchWeapon(int weaponIndex) {
    if (weaponIndex != PISTOL && weaponIndex != SHOTGUN) return;

    // Silah degisince pompalinin devam eden reload'i iptal edilir.
    shotgun.isReloading = false;
    CurrentWeapon = weaponIndex;
}

void ServerPlayer::ApplyActions(const PlayerActionCommand& actions) {
    if (actions.switchToWeapon >= 0) SwitchWeapon(actions.switchToWeapon);
    for (int i = 0; i < actions.reloadCount; i++) Reload();
    for (int i = 0; i < actions.attack1Count; i++) Attack(1);
    for (int i = 0; i < actions.attack2Count; i++) Attack(2);
}

void ServerPlayer::Update(float deltaTime) {
    shotgun.UpdateReload(deltaTime);
}

PlayerStats ServerPlayer::GetStats() const {
    const bool pistolSelected = (CurrentWeapon == PISTOL);

    PlayerStats stats;
    stats.health = Health;
    stats.maxHealth = maxHealth;
    stats.armor = Armor;
    stats.maxArmor = maxArmor;
    stats.currentWeapon = CurrentWeapon;
    stats.bullets = pistolSelected ? pistol.WBullet : shotgun.WBullet;
    stats.maxBullets = pistolSelected ? pistol.WmaxBullet : shotgun.WmaxBullet;
    stats.reserveAmmo = pistolSelected ? pistol.WAmmo : shotgun.WAmmo;
    stats.isReloading = !pistolSelected && shotgun.isReloading;
    stats.alive = Health > 0;
    return stats;
}

void ServerPlayer::Death() {
    // Olum davranisi (kamera, girdi kilidi, ekran yazisi) henuz tanimlanmadi.
}