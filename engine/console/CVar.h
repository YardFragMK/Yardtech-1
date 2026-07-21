#pragma once

class CVar {
public:
	bool cm_noclip = false;
	bool nvs_cheats = false;

	void cm_noclipF();
	void nvs_cheatsF();
};

extern CVar g_CVar;