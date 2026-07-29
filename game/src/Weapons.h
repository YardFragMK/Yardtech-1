#pragma once

//=========================================================
//Pistol
//=========================================================
class Pistol {
public:
	int Damage = 10;
	int maxBullet= 15;
	int Bullet = 100;
	int Ammo = 2;
	void Shoot() {
		if (Bullet > 0) {
			Bullet--;
		}
	}
	void Reload(){
		if (Ammo > 0) {
			Bullet = maxBullet;
			Ammo--;
		}
	}
};