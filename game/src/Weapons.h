#pragma once

//=========================================================
//Pistol
//=========================================================
class Pistol {
public:
	int Damage = 10;
	int maxBullet= 15;
	int Bullet = 10;
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

//=========================================================
//SHOTGUN
//=========================================================
class Shotgun {
public:
	int Damage = 20;
	int maxBullet = 7;
	int Bullet = 5;
	int Ammo = 2;
	void Shoot(int shootnumber) {
		if (shootnumber == 1) {
			if (Bullet > 0) {
				Bullet--;
			}
			else if (Bullet == 0) {
				//Boş mermi sesi
			}
		}
		else if (shootnumber == 2) {
			if (Bullet > 2) {
				Bullet = Bullet - 2;
			}
			else if (Bullet < 2 && Bullet >0) {
				Bullet--;
			}
			else if (Bullet == 0) {
				//Boş mermi sesi
			}
		}
	}

	void Reload() {
		if (Ammo > 0) {
			Bullet = maxBullet;
			Ammo--;
		}
	}
};