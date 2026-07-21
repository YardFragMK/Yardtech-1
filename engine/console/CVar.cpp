#include"CVar.h"
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