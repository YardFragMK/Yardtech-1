#pragma once
#include "playerstats.h"

// Client tarafindaki oyuncu "aynasi". Oyun durumunun sahibi server.dll'dir; bu sinif her
// tick'te server'dan gelen istatistikleri tutar ve HUD gibi tuketicilerin alanlari ve
// getter'lari okumasini saglar.
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
	float GetCharge() const { return m_charge; } // testere donusu / lastgun sarji, 0..1

	void ApplyStats(const PlayerStats& stats);

private:
	int m_bullets = 15;
	int m_maxBullets = 15;
	int m_reserveAmmo = 60;
	bool m_isReloading = false;
	float m_charge = 0.0f;
};

extern Player g_Player;