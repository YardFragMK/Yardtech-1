#include "Player.h"
#include "../../engine/LocalGame.h"

Player g_Player;

void Player::SwitchWeapon(int weaponIndex) {
	LocalGame_QueueSwitchWeapon(weaponIndex);
}

void Player::ReloadCurrentWeapon() {
	LocalGame_QueueReload();
}

void Player::attack(int attackNumber) {
	LocalGame_QueueAttack(attackNumber);
}

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
}