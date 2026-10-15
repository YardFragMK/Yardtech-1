#pragma once
#include "interface.h"
#include "iworldquery.h"
#include "netentity.h"
#include "PlayerMovement.h"

#define SERVER_GAME_DLL_INTERFACE_VERSION "ServerGameDLL001"
#define ENGINE_SERVER_INTERFACE_VERSION "EngineServer001"

// server.dll'in engine'den kullandigi hizmetler (exe veya dedicated exe uygular).
class IEngineServer {
public:
    virtual IWorldQuery* GetWorld() = 0;
    virtual bool IsDedicated() = 0;
    virtual int GetMaxClients() = 0;

    virtual void ChangeLevel(const char* mapName) = 0;
    virtual void DisconnectClient(int clientIndex, const char* reason) = 0;

    // Oyuna ozel mesajlar; engine icerigi yorumlamaz.
    virtual void SendMessageToClient(int clientIndex, const void* data, int size, bool reliable) = 0;
    virtual void BroadcastMessage(const void* data, int size, bool reliable) = 0;
};

// Engine'in server.dll'den bekledigi giris noktalari (server.dll uygular).
class IServerGameDLL {
public:
    virtual bool DLLInit(CreateInterfaceFn engineFactory) = 0;
    virtual void DLLShutdown() = 0;

    // Harita yuklendikten sonra cagrilir; entity listesi IWorldQuery'den okunur.
    virtual bool LevelInit(const char* mapName) = 0;
    virtual void LevelShutdown() = 0;

    virtual void GameFrame(float frameTime) = 0;

    virtual bool ClientConnect(int clientIndex, const char* playerName) = 0;
    virtual void ClientPutInServer(int clientIndex) = 0;
    virtual void ClientDisconnect(int clientIndex) = 0;

    virtual void RunPlayerCommand(int clientIndex, const PlayerInputCommand& command, float frameTime) = 0;
    virtual bool GetPlayerState(int clientIndex, PlayerPhysicsState* outState) = 0;

    // Replikasyon: engine her tick bu listeyi client'lara iletir.
    virtual int GetEntityStates(NetEntityState* outStates, int maxCount) = 0;
};