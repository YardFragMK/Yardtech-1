#pragma once
#include <glm/glm.hpp>
#include "interface.h"
#include "iworldquery.h"
#include "netentity.h"
#include "playerstats.h"
#include "PlayerMovement.h"

// Arayuz degistiginde versiyon artirilir; eski ve yeni modullerin yanlis
// eslesmesi GetInterface'in nullptr donmesiyle yakalanir.
#define SERVER_GAME_DLL_INTERFACE_VERSION "ServerGameDLL002"
#define ENGINE_SERVER_INTERFACE_VERSION "EngineServer001"

// server.dll'in engine'den kullandigi hizmetler (exe veya dedicated exe uygular).
class IEngineServer {
public:
    virtual IWorldQuery* GetWorld() = 0;
    virtual bool IsDedicated() = 0;
    virtual int GetMaxClients() = 0;

    virtual void ChangeLevel(const char* mapName) = 0;
    virtual void DisconnectClient(int clientIndex, const char* reason) = 0;

    virtual void SendMessageToClient(int clientIndex, const void* data, int size, bool reliable) = 0;
    virtual void BroadcastMessage(const void* data, int size, bool reliable) = 0;
};

// Engine'in server.dll'den bekledigi giris noktalari (server.dll uygular).
class IServerGameDLL {
public:
    virtual bool DLLInit(CreateInterfaceFn engineFactory) = 0;
    virtual void DLLShutdown() = 0;

    virtual bool LevelInit(const char* mapName) = 0;
    virtual void LevelShutdown() = 0;

    virtual void GameFrame(float frameTime) = 0;

    virtual bool ClientConnect(int clientIndex, const char* playerName) = 0;
    virtual void ClientPutInServer(int clientIndex) = 0;
    virtual void ClientDisconnect(int clientIndex) = 0;

    // Oyuncu hareket simulasyonu: Faz 3'te server'a tasinacak, simdilik kullanilmiyor.
    virtual void RunPlayerCommand(int clientIndex, const PlayerInputCommand& command, float frameTime) = 0;
    virtual bool GetPlayerState(int clientIndex, PlayerPhysicsState* outState) = 0;

    // Faz 2a: hareket server'a tasinana kadar, host oyuncunun goz konumunu ve
    // hizini server'a bildirir (botlarin hedefleme/isabet hesaplari icin).
    virtual void SetPlayerView(int clientIndex, const glm::vec3& eyePosition, float moveSpeed) = 0;
    virtual void RunPlayerActions(int clientIndex, const PlayerActionCommand& actions) = 0;
    virtual bool GetPlayerStats(int clientIndex, PlayerStats* outStats) = 0;

    virtual int GetEntityStates(NetEntityState* outStates, int maxCount) = 0;
};