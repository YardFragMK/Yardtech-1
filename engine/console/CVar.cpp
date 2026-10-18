#include"CVar.h"
#include"Console.h"
#include"../Engine.h"
#include"../HUD.h"
#include"../ConVarRegistry.h"

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
void CVar::nvs_developerF(int developer) {
	nvs_developer = developer;
	Console::Log("nvs_developer set to " + std::to_string(nvs_developer));
}

void CVar::helpF() const {
	Console::Log("nvs_cheats " + std::to_string(nvs_cheats));
	Console::Log("nvs_developer " + std::to_string(nvs_developer));
	Console::Log("cm_noclip " + std::to_string(cm_noclip));
	Console::Log("nvs_gravity " + std::to_string(nvs_gravity));
	Console::Log("nvs_jumpforce " + std::to_string(nvs_jumpforce));
	Console::Log("nvs_doombarenabled " + std::to_string(HUD::doomBarEnabled));

	// client.dll ve diger modullerin kaydettigi degiskenler
	ConVarRegistry::PrintAll();
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

void CVar::nvs_doombarenabledF(int value) {
	HUD::doomBarEnabled = value;
	Console::Log("nvs_doombarenabled set to " + std::to_string(value));
}