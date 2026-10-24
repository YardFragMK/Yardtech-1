#include "Player.h"

Player g_Player;

void Player::ApplyStats(const PlayerStats& stats) {
	Health = stats.health;
	maxHealth = stats.maxHealth;
	Armor = stats.armor;
	maxArmor = stats.maxArmor;
	CurrentWeapon = stats.currentWeapon;
	m_bullets = stats.bullets;
	m_maxBullets = stats.maxBullets;
	m_reserveAmmo = stats.reserveAmmo;
	m_isReloading = stats.isReloading;
	m_charge = stats.charge;
}