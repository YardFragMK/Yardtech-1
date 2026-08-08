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
	int Health = 100;
	int Armor = 0;
	int Damage;
	int CurrentWeapon = PISTOL;

	void takeDamage(int enemyDamage);
	void attack(int attacknumber);
private:
	Pistol pistol;
	Shotgun shotgun;
};
