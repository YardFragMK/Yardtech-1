#pragma once
#include <vector>
#include "netentity.h"

// Tek oyunculu/yerel oyunda exe ile server.dll arasindaki kopru. Her sabit
// tick'te oyuncunun goz konumunu ve silah eylemlerini server'a iletir,
// server'in GameFrame'ini calistirir, ardindan istatistikleri ve entity
// listesini geri alir. Bir sunucuya baglanildiginda kullanilmaz.
void LocalGame_Reset();
void LocalGame_Tick(float frameTime);
const std::vector<NetEntityState>& LocalGame_GetEntities();

void LocalGame_QueueAttack(int attackNumber);
void LocalGame_QueueReload();
void LocalGame_QueueSwitchWeapon(int weaponIndex);