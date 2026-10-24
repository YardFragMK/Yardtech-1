#pragma once
#include <vector>
#include "netentity.h"

// Oyuncu silahlarinin mermilerini, isinlarini, patlamalarini, sis bulutunu ve lav
// havuzunu cizer. Sinif kimligi ENT_CLASS_ROCKET ve uzeri olan entity'ler islenir.
void WeaponEffects_Render(const std::vector<NetEntityState>& entities);