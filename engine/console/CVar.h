#pragma once

class CVar {
public:
	bool cm_noclip = false;
	bool nvs_cheats = true;
	int nvs_developer = 1;
	float nvs_gravity = 900.0f;   // unit/s^2,
	float nvs_jumpforce = 250.0f; // unit/s, ziplama baslangic dikey hizi
	//bool itemRGD = true;


	void cm_noclipF(int cm_noclipValue);
	void nvs_cheatsF(int nvs_cheatsValue);
	void cm_speedF(float cm_speed) const;
	void cm_sensitivityF(float cm_sensitivity) const;
	void cm_rollmaxF(float cm_rollmax) const;
	void cm_rollspeedF(float cm_rollspeed) const;
	void nvs_developerF(int developer);
	void nvs_gravityF(float value);   
	void nvs_jumpforceF(float value);
	void helpF() const;
	void r_retromodeF(int r_retromodeValue);
};

extern CVar g_CVar;