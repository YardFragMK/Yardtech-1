#pragma once

// Engine'e (exe) ait degiskenler. Kamera/girdi ayarlari (cm_sensitivity, cm_speed
// vb.) client.dll'in kaydettigi degiskenlerdir ve ConVarRegistry'de yasar.
class CVar {
public:
	bool cm_noclip = false;
	bool nvs_cheats = true;
	int nvs_developer = 1;
	float nvs_gravity = 900.0f;   // unit/s^2
	float nvs_jumpforce = 250.0f; // unit/s, ziplama baslangic dikey hizi
	float nvs_mastervolume = 1.0f; // 0.0-1.0
	float nvs_musicvolume = 1.0f;  // 0.0-1.0

	void cm_noclipF(int cm_noclipValue);
	void nvs_cheatsF(int nvs_cheatsValue);
	void nvs_developerF(int developer);
	void nvs_gravityF(float value);
	void nvs_jumpforceF(float value);
	void helpF() const;
	void r_retromodeF(int r_retromodeValue);
	void nvs_doombarenabledF(int value);
};

extern CVar g_CVar;