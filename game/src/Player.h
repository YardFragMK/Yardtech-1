#pragma once
#include"Weapons.h"
#include"../../engine/console/Console.h"

enum Weapon {
	PISTOL=0,
	SHOTGUN=1
};

class Player {
public:
	int maxHealth = 200;
	int maxArmor = 200;
	int Health = 120;
	int Armor = 100;
	int Damage;
	int CurrentWeapon = PISTOL;
	bool RGDitem = true;

	int GetCurrentBullet() const {
		return (CurrentWeapon == PISTOL) ? pistol.WBullet : shotgun.WBullet;
	}
	int GetCurrentMaxBullet() const {
		return (CurrentWeapon == PISTOL) ? pistol.WmaxBullet : shotgun.WmaxBullet;
	}
	int GetCurrentAmmo() const {
		return (CurrentWeapon == PISTOL) ? pistol.WAmmo : shotgun.WAmmo;
	}

	void takeDamage(int enemyDamage);
	void attack(int attacknumber);
	void death();

	// --- silah gecisi ---
	void SwitchWeapon(int weaponIndex) {
		if (weaponIndex != PISTOL && weaponIndex != SHOTGUN) return;
		//if (CurrentWeapon == weaponIndex) return;

		// silah degisirken pompalinin devam eden reload'i varsa iptal et
		// (klasik FPS davranisi: silah degistirince reload kesilir)
		shotgun.isReloading = false;

		CurrentWeapon = weaponIndex;
	}

	// --- reload baslat (R tusu) ---
	void ReloadCurrentWeapon() {
		if (CurrentWeapon == PISTOL) {
			pistol.Reload(); 
		}
		else {
			shotgun.StartReload(); // zamanli, tek tek
		}
	}

	// --- her frame cagrilmali ---
	void UpdateWeapons(float deltaTime) {
		shotgun.UpdateReload(deltaTime);
	}
private:
	Pistol pistol;
	Shotgun shotgun;
};
extern Player g_Player;