#include"CVar.h"
#include"../Camera.h"
#include<iostream>

CVar g_CVar;

void CVar::cm_noclipF() {
	if (nvs_cheats == true) {
		cm_noclip = !cm_noclip;
		std::cout << "cm_noclip " << cm_noclip << std::endl;
	}
}
void CVar::nvs_cheatsF() {
	nvs_cheats = !nvs_cheats;
	std::cout << "nvs_cheats " << nvs_cheats << std::endl;
}
void CVar::cm_speedF(float cm_speed) {
	if (nvs_developer == 1) {
		g_Camera.moveSpeed = cm_speed;
		std::cout << "cm_speed set to " << cm_speed << std::endl;
	}
	else {
		std::cout << "cm_speed: developer mode required\n";
	}
}
void CVar::cm_sensitivityF(float cm_sensitivity) {
	g_Camera.mouseSensitivity = cm_sensitivity;
	std::cout << "cm_sensitivity set to " << cm_sensitivity << std::endl;
}
void CVar::cm_rollmaxF(float cm_rollmax) {
	if (nvs_developer == 1) {
		g_Camera.maxRoll = cm_rollmax;
		std::cout << "cm_rollmax set to " << cm_rollmax << std::endl;
	}
	else {
		std::cout << "cm_rollmax: developer mode required\n";
	}
}
void CVar::cm_rollspeedF(float cm_rollspeed) {
	if (nvs_developer == 1) {
		g_Camera.rollLerpSpeed = cm_rollspeed;
		std::cout << "cm_rollspeed set to " << cm_rollspeed << std::endl;
	}
	else {
		std::cout << "cm_rollspeed: developer mode required\n";
	}
}
void CVar::nvs_developerF(int developer) {
	nvs_developer = developer;
	std::cout << "nvs_developer set to " << nvs_developer << std::endl;
}

