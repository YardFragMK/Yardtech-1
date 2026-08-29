#pragma once

//=========================================================
// WEAPON BASE
//=========================================================
class WeaponBase {
public:
	int WDamage = 1;
	int WmaxBullet = 1;
	int WBullet = 1;
	int WAmmo = 1;

	virtual ~WeaponBase() = default;
	virtual void Shoot(int shootnumber)=0;
};


//=========================================================
//Pistol
//=========================================================
class Pistol : public WeaponBase{
public:
	int WDamage = 10;
	int WmaxBullet = 15;
	int WBullet = 10;
	int WAmmo = 2;

	void Shoot(int shootnumber) override {
		if (WBullet > 0) {
			WBullet--;
		}
		else {
			if (WAmmo > 0) {
				Reload();
			}
		}
	}
	void Reload(){
		if (WBullet == WmaxBullet) return;
		if (WAmmo > 0) {
			WBullet = WmaxBullet;
			WAmmo--;
		}
	}
};

//=========================================================
//SHOTGUN
//=========================================================
class Shotgun : public WeaponBase {
public:
	int WDamage = 20;
	int WmaxBullet = 7;
	int WBullet = 5;
	int WAmmo = 2;

	// --- tek-tek, zamanli reload state'i ---
	bool isReloading = false;
	float reloadTimer = 0.0f;
	static constexpr float RELOAD_SHELL_INTERVAL = 0.5f; // her mermi arasi bekleme

	void Shoot(int shootnumber) override {
		// reload sirasinda ates etmeyi iptal et (klasik pompali davranisi)
		if (isReloading) return;

		if (shootnumber == 1) {
			if (WBullet > 0) {
				WBullet--;
			}
			else if (WBullet == 0) {
				if (WAmmo > 0) {
					StartReload();
				}
				else {
					//Boş mermi sesi
				}

			}
		}
		else if (shootnumber == 2) {
			if (WBullet > 2) {
				WBullet = WBullet - 2;
			}
			else if (WBullet < 2 && WBullet >0) {
				WBullet--;
			}
			else if (WBullet == 0) {
				if (WAmmo > 0) {
					StartReload();
				}
				else {
					//Boş mermi sesi
				}
			}
		}
	}

	// Reload baslat (tek seferlik cagri, R tusuna basildiginda)
	void StartReload() {
		if (isReloading) return;
		if (WBullet >= WmaxBullet) return; // sarjor zaten dolu
		if (WAmmo <= 0) return;           // yedek yok
		isReloading = true;
		reloadTimer = 0.0f;
	}

	// Her frame cagrilmali (deltaTime ile). Reload devam ediyorsa 0.5 saniyede
	// bir mermi ekler, maxBullet'e ya da Ammo bitene kadar devam eder.
	void UpdateReload(float deltaTime) {
		if (!isReloading) return;

		reloadTimer += deltaTime;
		if (reloadTimer >= RELOAD_SHELL_INTERVAL) {
			reloadTimer -= RELOAD_SHELL_INTERVAL;

			if (WBullet < WmaxBullet && WAmmo > 0) {
				WBullet++;
				WAmmo--;
			}

			// sarjor doldu ya da yedek bitti -> reload'i sonlandir
			if (WBullet >= WmaxBullet || WAmmo <= 0) {
				isReloading = false;
			}
		}
	}
};