#include "LocalGame.h"
#include "ModuleLoader.h"
#include "Camera.h"
#include "../game/src/Player.h"
#include "playerstats.h"

namespace {
	constexpr int MAX_LOCAL_ENTITIES = 512;
	constexpr int LOCAL_CLIENT_INDEX = 0;

	PlayerActionCommand s_pendingActions;
	std::vector<NetEntityState> s_entities;
}

void LocalGame_Reset() {
	s_pendingActions = PlayerActionCommand();
	s_entities.clear();
	g_Player = Player();
}

void LocalGame_QueueAttack(int attackNumber) {
	if (attackNumber == 1) s_pendingActions.attack1Count++;
	else if (attackNumber == 2) s_pendingActions.attack2Count++;
}

void LocalGame_QueueReload() {
	s_pendingActions.reloadCount++;
}

void LocalGame_QueueSwitchWeapon(int weaponIndex) {
	s_pendingActions.switchToWeapon = weaponIndex;
}

void LocalGame_Tick(float frameTime) {
	IServerGameDLL* server = GetServerDLL();
	if (server == nullptr) return;

	server->SetPlayerView(LOCAL_CLIENT_INDEX, g_Camera.GetEyePosition(), g_Camera.moveSpeed);
	server->RunPlayerActions(LOCAL_CLIENT_INDEX, s_pendingActions);
	s_pendingActions = PlayerActionCommand();

	server->GameFrame(frameTime);

	PlayerStats stats;
	if (server->GetPlayerStats(LOCAL_CLIENT_INDEX, &stats)) {
		g_Player.ApplyStats(stats);
	}

	NetEntityState buffer[MAX_LOCAL_ENTITIES];
	int count = server->GetEntityStates(buffer, MAX_LOCAL_ENTITIES);
	s_entities.assign(buffer, buffer + count);
}

const std::vector<NetEntityState>& LocalGame_GetEntities() {
	return s_entities;
}