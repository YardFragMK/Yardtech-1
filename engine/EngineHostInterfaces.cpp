#include "interface.h"
#include "iclientdll.h"
#include "iservergamedll.h"
#include "iworldquery.h"
#include "InputBridge.h"
#include "Level.h"
#include "GameState.h"
#include "NetClient.h"
#include "BSPMap.h"
#include "console/Console.h"
#include "../game/src/BotManager.h"
#include <SDL.h>

// Exe'nin (oyun host'unun) DLL'lere sundugu IWorldQuery, IEngineClient ve
// IEngineServer gerceklestirmeleri. Dedicated server exe'si kendi
// IEngineServer/IWorldQuery gerceklestirmelerini ayri olarak yazar.

class WorldQueryImpl : public IWorldQuery {
public:
    bool IsLoaded() const override { return !GetCurrentLevelName().empty(); }
    const char* GetMapName() const override { return GetCurrentLevelName().c_str(); }

    TraceResult TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex) const override {
        return g_Map.TraceLine(start, end, hullIndex);
    }
    glm::vec3 SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex) const override {
        return g_Map.SlideMove(start, end, hullIndex);
    }
    bool IsPointSolid(const glm::vec3& position, int hullIndex) const override {
        return g_Map.IsPointSolid(position, hullIndex);
    }

    int GetEntityCount() const override {
        return static_cast<int>(g_Map.GetEntities().size());
    }
    const char* GetEntityKeyValue(int entityIndex, const char* key) const override {
        const auto& entities = g_Map.GetEntities();
        if (entityIndex < 0 || entityIndex >= static_cast<int>(entities.size()) || key == nullptr) return nullptr;

        const std::string* value = entities[entityIndex].Get(key);
        return value != nullptr ? value->c_str() : nullptr;
    }
};
EXPOSE_SINGLE_INTERFACE(WorldQueryImpl, IWorldQuery, WORLD_QUERY_INTERFACE_VERSION);

static IWorldQuery* GetWorldQuery() {
    return GetInterface<IWorldQuery>(&CreateInterface, WORLD_QUERY_INTERFACE_VERSION);
}

class EngineClientImpl : public IEngineClient {
public:
    void GetScreenSize(int* width, int* height) override {
        int w = 0, h = 0;
        if (SDL_Window* window = SDL_GL_GetCurrentWindow()) SDL_GetWindowSize(window, &w, &h);
        if (width) *width = w;
        if (height) *height = h;
    }

    bool IsButtonDown(int buttonCode) override { return IsButtonCodeDown(buttonCode); }

    void SetMouseCapture(bool capture) override {
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
        SDL_ShowCursor(capture ? SDL_DISABLE : SDL_ENABLE);
    }

    IWorldQuery* GetWorld() override { return GetWorldQuery(); }
    const char* GetLevelName() override { return GetCurrentLevelName().c_str(); }
    bool IsInGame() override { return g_State == GameState::Playing || g_State == GameState::Paused; }
    bool IsConnectedToServer() override { return NetClient::IsConnected(); }

    void StartListenServer(const char* mapName) override {
        if (mapName == nullptr) return;
        if (LoadLevel(mapName)) {
            BotManager::Init(); // bot yonetimi server.dll'e tasininca bu satir kalkacak
            EnterPlaying();
        }
    }

    void ConnectToServer(const char* address) override {
        if (address != nullptr) NetClient::Connect(address);
    }

    void Disconnect() override { NetClient::Disconnect(); }

    void ClientCmd(const char* command) override {
        // Konsol komut ayristiricisina baglanma Faz 2'de yapilacak.
        Console::Log(std::string("ClientCmd henuz baglanmadi: ") + (command != nullptr ? command : ""));
    }

    void Quit() override {
        SDL_Event quitEvent{};
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
    }
};
EXPOSE_SINGLE_INTERFACE(EngineClientImpl, IEngineClient, ENGINE_CLIENT_INTERFACE_VERSION);

class EngineServerImpl : public IEngineServer {
public:
    IWorldQuery* GetWorld() override { return GetWorldQuery(); }
    bool IsDedicated() override { return false; }
    int GetMaxClients() override { return 1; } // listen server henuz yok

    void ChangeLevel(const char* mapName) override {
        if (mapName != nullptr) LoadLevel(mapName);
    }

    void DisconnectClient(int, const char* reason) override {
        Console::Log(std::string("DisconnectClient henuz baglanmadi: ") + (reason != nullptr ? reason : ""));
    }

    // Oyun mesajlari Faz 3'te yerel loopback ve ENet'e baglanacak.
    void SendMessageToClient(int, const void*, int, bool) override {}
    void BroadcastMessage(const void*, int, bool) override {}
};
EXPOSE_SINGLE_INTERFACE(EngineServerImpl, IEngineServer, ENGINE_SERVER_INTERFACE_VERSION);