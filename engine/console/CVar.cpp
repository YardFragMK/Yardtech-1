#include"CVar.h"
#include"../Camera.h"
#include"Console.h"
#include"../Engine.h"
#include"../HUD.h"

CVar g_CVar;

void CVar::cm_noclipF(int cm_noclipValue) {
	if (nvs_cheats == true) {
		cm_noclip = cm_noclipValue;
		Console::Log("cm_noclip set to " + std::to_string(cm_noclip));
	}
}
void CVar::nvs_cheatsF(int nvs_cheatsValue) {
	nvs_cheats = nvs_cheatsValue;
	Console::Log("nvs_cheats set to " + std::to_string(nvs_cheats));
}
void CVar::cm_speedF(float cm_speed) const {
	if (nvs_developer == 1) {
		g_Camera.moveSpeed = cm_speed;
		Console::Log("cm_speed set to " + std::to_string(cm_speed));
	}
	else {
		Console::Log("cm_speed: developer mode required");
	}
}
void CVar::cm_sensitivityF(float cm_sensitivity) const {
	g_Camera.mouseSensitivity = cm_sensitivity;
	Console::Log("cm_sensitivity set to " + std::to_string(cm_sensitivity));
}
void CVar::cm_rollmaxF(float cm_rollmax) const {
	if (nvs_developer == 1) {
		g_Camera.maxRoll = cm_rollmax;
		Console::Log("cm_rollmax set to " + std::to_string(cm_rollmax));
	}
	else {
		Console::Log("cm_rollmax: developer mode required");
	}
}
void CVar::cm_rollspeedF(float cm_rollspeed) const {
	if (nvs_developer == 1) {
		g_Camera.rollLerpSpeed = cm_rollspeed;
		Console::Log("cm_rollspeed set to " + std::to_string(cm_rollspeed));
	}
	else {
		Console::Log("cm_rollspeed: developer mode required");
	}
}
void CVar::nvs_developerF(int developer) {
	nvs_developer = developer;
	Console::Log("nvs_developer set to " + std::to_string(nvs_developer));
}

void CVar::helpF() const {
	Console::Log("nvs_cheats " + std::to_string(nvs_cheats));
	Console::Log("nvs_developer " + std::to_string(nvs_developer));
	Console::Log("cm_noclip " + std::to_string(cm_noclip));
	Console::Log("cm_sensitivity " + std::to_string(g_Camera.mouseSensitivity));
	Console::Log("nvs_gravity " + std::to_string(nvs_gravity));       
	Console::Log("nvs_jumpforce " + std::to_string(nvs_jumpforce));  
	Console::Log("cm_viewbobstyle " + std::to_string(g_Camera.viewBobStyle));
	Console::Log("nvs_doombarenabled " + std::to_string(HUD::doomBarEnabled));
	if (nvs_developer == 1) {
		Console::Log("cm_speed " + std::to_string(g_Camera.moveSpeed));
		Console::Log("cm_rollmax " + std::to_string(g_Camera.maxRoll));
		Console::Log("cm_rollspeed " + std::to_string(g_Camera.rollLerpSpeed));
	}
}

void CVar::r_retromodeF(int r_retromodeValue) {
	renderer.SetRetroMode(r_retromodeValue);
}

void CVar::nvs_gravityF(float value) {
	if (nvs_cheats == true) {
		nvs_gravity = value;
		Console::Log("nvs_gravity set to " + std::to_string(nvs_gravity));
	}
}

void CVar::nvs_jumpforceF(float value) {
	if (nvs_cheats == true) {
		nvs_jumpforce = value;
		Console::Log("nvs_jumpforce set to " + std::to_string(nvs_jumpforce));
	}
}

void CVar::cm_viewbobstyleF(int value) {
	g_Camera.viewBobStyle = value;
	Console::Log("cm_viewbobstyle set to " + std::to_string(value));
}

void CVar::nvs_doombarenabledF(int value) {
	HUD::doomBarEnabled = value;
	Console::Log("nvs_doombarenabled set to " + std::to_string(value));
}