#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "netentity.h"
#include "playerstats.h"

// Tek oyunculu/yerel oyunda exe ile server.dll arasindaki kopru. Her sabit
// tick'te oyuncunun goz konumunu ve silah eylemlerini server'a iletir,
// server'in GameFrame'ini calistirir, ardindan istatistikleri ve entity
// listesini hem exe tarafina hem client.dll'e verir.
void LocalGame_Reset();
void LocalGame_Tick(float frameTime, const PlayerActionCommand& actions,
	const glm::vec3& eyePosition, float moveSpeed);
const std::vector<NetEntityState>& LocalGame_GetEntities();