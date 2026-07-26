#pragma once

class CVar {
public:
	bool cm_noclip = false;
	bool nvs_cheats = true;
	int nvs_developer = 1;


	void cm_noclipF();
	void nvs_cheatsF();
	void cm_speedF(float cm_speed) const;
	void cm_sensitivityF(float cm_sensitivity) const;
	void cm_rollmaxF(float cm_rollmax) const;
	void cm_rollspeedF(float cm_rollspeed) const;
	void nvs_developerF(int developer);
	void helpF() const;
};

extern CVar g_CVar;