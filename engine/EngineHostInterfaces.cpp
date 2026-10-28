#include "interface.h"
#include "iclientdll.h"
#include "iservergamedll.h"
#include "iworldquery.h"
#include "InputBridge.h"
#include "Level.h"
#include "GameState.h"
#include "NetClient.h"
#include "BSPMap.h"
#include "Settings.h"
#include "ServerMenu.h"
#include "console/Console.h"
#include "console/CVar.h"
#include "Engine.h"
#include <cstring>
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

    bool GetBrushModelBounds(int modelIndex, glm::vec3* outMins, glm::vec3* outMaxs) const override {
        return g_Map.GetModelBounds(modelIndex, outMins, outMaxs);
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

    bool IsGameplayInputActive() override {
        return g_State == GameState::Playing
            && !Console::IsOpen()
            && !Settings::IsOpen()
            && !ServerMenu::IsOpen();
    }

    IWorldQuery* GetWorld() override { return GetWorldQuery(); }
    const char* GetLevelName() override { return GetCurrentLevelName().c_str(); }
    bool IsInGame() override { return g_State == GameState::Playing || g_State == GameState::Paused; }
    bool IsConnectedToServer() override { return NetClient::IsConnected(); }

    float GetCvarFloat(const char* name, float defaultValue) override {
        if (name == nullptr) return defaultValue;
        if (std::strcmp(name, "nvs_gravity") == 0) return g_CVar.nvs_gravity;
        if (std::strcmp(name, "nvs_jumpforce") == 0) return g_CVar.nvs_jumpforce;
        if (std::strcmp(name, "nvs_mastervolume") == 0) return g_CVar.nvs_mastervolume;
        if (std::strcmp(name, "nvs_musicvolume") == 0) return g_CVar.nvs_musicvolume;
        return defaultValue;
    }

    int GetCvarInt(const char* name, int defaultValue) override {
        if (name == nullptr) return defaultValue;
        if (std::strcmp(name, "cm_noclip") == 0) return g_CVar.cm_noclip ? 1 : 0;
        if (std::strcmp(name, "nvs_cheats") == 0) return g_CVar.nvs_cheats ? 1 : 0;
        if (std::strcmp(name, "nvs_developer") == 0) return g_CVar.nvs_developer;
        return defaultValue;
    }

    void StartListenServer(const char* mapName) override {
        if (mapName == nullptr) return;
        if (LoadLevel(mapName)) {
            EnterPlaying();
        }
    }

    void ConnectToServer(const char* address) override {
        if (address != nullptr) NetClient::Connect(address);
    }

    void Disconnect() override { NetClient::Disconnect(); }

    void ClientCmd(const char* command) override {
        if (command != nullptr) Console::ExecuteLine(command);
    }

    void Quit() override {
        SDL_Event quitEvent{};
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
    }

    int GetFileSize(const char* vfsPath) override {
        if (vfsPath == nullptr) return -1;
        return static_cast<int>(vfs.GetFileSize(vfsPath));
    }

    bool ReadFile(const char* vfsPath, void* buffer, int bufferSize) override {
        if (vfsPath == nullptr || buffer == nullptr || bufferSize <= 0) return false;

        const std::vector<uint8_t> data = vfs.ReadFile(vfsPath);
        if (data.empty() || data.size() > static_cast<size_t>(bufferSize)) return false;

        std::memcpy(buffer, data.data(), data.size());
        return true;
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

    // Oyun mesajlari, yerel loopback ve ENet'e baglanana kadar bos birakildi.
    void SendMessageToClient(int, const void*, int, bool) override {}
    void BroadcastMessage(const void*, int, bool) override {}
};
EXPOSE_SINGLE_INTERFACE(EngineServerImpl, IEngineServer, ENGINE_SERVER_INTERFACE_VERSION);