#pragma once

class CVar {
public:
	bool cm_noclip = true;
	bool nvs_cheats = false;
	int nvs_developer = 0;


	void cm_noclipF();
	void nvs_cheatsF();
	void cm_speedF(float cm_speed);
	void cm_sensitivityF(float cm_sensitivity);
	void cm_rollmaxF(float cm_rollmax);
	void cm_rollspeedF(float cm_rollspeed);
	void nvs_developerF(int developer);
};

extern CVar g_CVar;