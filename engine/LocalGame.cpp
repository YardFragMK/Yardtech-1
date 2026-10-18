#include "LocalGame.h"
#include "ModuleLoader.h"
#include "../game/src/Player.h"

namespace {
	constexpr int MAX_LOCAL_ENTITIES = 512;
	constexpr int LOCAL_CLIENT_INDEX = 0;

	std::vector<NetEntityState> s_entities;
}

void LocalGame_Reset() {
	s_entities.clear();
	g_Player = Player();
}

void LocalGame_Tick(float frameTime, const PlayerActionCommand& actions,
	const glm::vec3& eyePosition, float moveSpeed) {
	IServerGameDLL* server = GetServerDLL();
	if (server == nullptr) return;

	IBaseClientDLL* client = GetClientDLL();

	server->SetPlayerView(LOCAL_CLIENT_INDEX, eyePosition, moveSpeed);
	server->RunPlayerActions(LOCAL_CLIENT_INDEX, actions);
	server->GameFrame(frameTime);

	PlayerStats stats;
	if (server->GetPlayerStats(LOCAL_CLIENT_INDEX, &stats)) {
		g_Player.ApplyStats(stats);
		if (client != nullptr) client->ReceivePlayerStats(stats);
	}

	NetEntityState buffer[MAX_LOCAL_ENTITIES];
	int count = server->GetEntityStates(buffer, MAX_LOCAL_ENTITIES);
	s_entities.assign(buffer, buffer + count);
	if (client != nullptr) client->ReceiveEntityStates(buffer, count);
}

const std::vector<NetEntityState>& LocalGame_GetEntities() {
	return s_entities;
}