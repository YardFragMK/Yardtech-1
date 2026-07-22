#pragma once

class CVar {
public:
	bool cm_noclip = false;
	bool nvs_cheats = false;



	void cm_noclipF();
	void nvs_cheatsF();
	void cm_speedF(float cm_speed);
	void cm_sensitivityF(float cm_sensitivity);
	void cm_rollmaxF(float cm_rollmax);
	void cm_rollspeedF(float cm_rollspeed);
};

extern CVar g_CVar;