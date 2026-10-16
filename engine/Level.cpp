#include "Level.h"
#include "Engine.h"
#include "MapLoader.h"
#include "ModuleLoader.h"
#include "LocalGame.h"
#include "Logger.h"
#include <vector>

namespace {
    std::string s_levelName;
    bool s_serverLevelActive = false;
}

const std::string& GetCurrentLevelName() {
    return s_levelName;
}

void UnloadLevel() {
    if (s_levelName.empty()) return;

    if (IBaseClientDLL* client = GetClientDLL()) client->LevelShutdown();

    if (s_serverLevelActive) {
        if (IServerGameDLL* server = GetServerDLL()) {
            server->ClientDisconnect(0);
            server->LevelShutdown();
        }
        s_serverLevelActive = false;
    }

    s_levelName.clear();
    LocalGame_Reset();
}

bool LoadLevel(const std::string& mapName, bool runLocalServer) {
    UnloadLevel();

    std::vector<uint8_t> buffer = vfs.ReadFile("map/" + mapName + ".bsp");
    if (buffer.empty() || !LoadMapFromMemory(buffer, mapName)) {
        Logger::error("Harita yuklenemedi: " + mapName);
        return false;
    }

    LocalGame_Reset();

    // Seviye adi server baslatilmadan once atanir: IWorldQuery::GetMapName
    // server'in LevelInit'i sirasinda da gecerli olmalidir.
    s_levelName = mapName;

    if (runLocalServer) {
        if (IServerGameDLL* server = GetServerDLL()) {
            if (!server->LevelInit(mapName.c_str())) {
                Logger::error("server.dll LevelInit basarisiz: " + mapName);
                s_levelName.clear();
                return false;
            }

            // Yerel oyunda tek oyuncu 0 numarali slota baglanir.
            server->ClientConnect(0, "Player");
            server->ClientPutInServer(0);
            s_serverLevelActive = true;
        }
    }

    if (IBaseClientDLL* client = GetClientDLL()) {
        client->LevelInit(mapName.c_str());
    }
    return true;
}