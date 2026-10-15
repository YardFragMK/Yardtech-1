#include "Level.h"
#include "Engine.h"
#include "MapLoader.h"
#include "ModuleLoader.h"
#include "Logger.h"
#include <vector>

using IClientDLLPtr = IBaseClientDLL*;
using IServerDLLPtr = IServerGameDLL*;

namespace {
    std::string s_levelName;
}

const std::string& GetCurrentLevelName() {
    return s_levelName;
}

void UnloadLevel() {
    if (s_levelName.empty()) return;

    if (IClientDLLPtr client = GetClientDLL()) client->LevelShutdown();
    if (IServerDLLPtr server = GetServerDLL()) server->LevelShutdown();

    s_levelName.clear();
}

bool LoadLevel(const std::string& mapName) {
    UnloadLevel();

    std::vector<uint8_t> buffer = vfs.ReadFile("map/" + mapName + ".bsp");
    if (buffer.empty() || !LoadMapFromMemory(buffer, mapName)) {
        Logger::error("Harita yuklenemedi: " + mapName);
        return false;
    }

    // Server once baslatilir: client, LevelInit sirasinda dunyaya ve
    // server'in urettigi entity'lere guvenebilmelidir.
    if (IServerDLLPtr server = GetServerDLL()) {
        if (!server->LevelInit(mapName.c_str())) {
            Logger::error("server.dll LevelInit basarisiz: " + mapName);
            return false;
        }
    }

    s_levelName = mapName;

    if (IClientDLLPtr client = GetClientDLL()) {
        client->LevelInit(mapName.c_str());
    }
    return true;
}