#pragma once
#include <glm/glm.hpp>
#include "ienginecommon.h"
#include "iservergamedll.h"
#include "ServerPlayer.h"

struct PlayerSlot {
    bool connected = false;
    ServerPlayer player;
    glm::vec3 eyePosition{ 0.0f };
    float moveSpeed = 320.0f;
};

// server.dll'in modul-ici paylasilan durumu: engine arayuzleri ve oyuncu slotlari.
// Yalnizca bu DLL'in icinden erisilir; baska modullerle paylasilmaz.
class ServerContext {
public:
    static constexpr int MAX_PLAYERS = 16;

    IEngineConsole* console = nullptr;
    IEngineServer* engine = nullptr;
    IWorldQuery* world = nullptr;
    PlayerSlot slots[MAX_PLAYERS];

    void Log(const char* message) const;
    PlayerSlot* GetSlot(int clientIndex);
    // Bagli oyuncular arasinda verilen noktaya en yakin olani; yoksa nullptr.
    PlayerSlot* FindNearestPlayer(const glm::vec3& from);
};

ServerContext& Ctx();