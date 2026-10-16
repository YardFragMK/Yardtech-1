#pragma once
#include "playerstats.h"

// Client tarafindaki oyuncu "aynasi". Oyun durumunun sahibi artik server.dll'dir;
// bu sinif her tick'te server'dan gelen istatistikleri tutar ve HUD gibi
// tuketicilerin eski arayuzu (alanlar ve getter'lar) kullanmasini saglar.
// Silah eylemleri dogrudan uygulanmaz, server'a iletilmek uzere kuyruga alinir.
class Player {
public:
	int maxHealth = 200;
	int maxArmor = 200;
	int Health = 200;
	int Armor = 200;
	int CurrentWeapon = PISTOL;
	bool RGDitem = true; // client tarafi ozelligi: havada sert inis izni

	int GetCurrentBullet() const { return m_bullets; }
	int GetCurrentMaxBullet() const { return m_maxBullets; }
	int GetCurrentAmmo() const { return m_reserveAmmo; }
	bool IsReloading() const { return m_isReloading; }

	void SwitchWeapon(int weaponIndex);
	void ReloadCurrentWeapon();
	void attack(int attackNumber);

	// Silah zamanlayicilari server'da calisir; ayna tarafinda yapilacak is yok.
	void UpdateWeapons(float deltaTime) { (void)deltaTime; }

	void ApplyStats(const PlayerStats& stats);

private:
	int m_bullets = 10;
	int m_maxBullets = 15;
	int m_reserveAmmo = 10;
	bool m_isReloading = false;
};

extern Player g_Player;