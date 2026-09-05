#include "MapLoader.h"
#include "BSPMap.h"
#include "Camera.h"
#include "Logger.h"
#include "console/Console.h"
#include "EntityParser.h"
#include <cstdlib>
#include "Skybox.h"

bool LoadMap(const std::string& bspPath) {
    if (!g_Map.Load("nvs1/map/" + bspPath + ".bsp", {"", "map/", "wads/", "textures/"})) {
        Logger::error("BSP yuklenemedi: " + bspPath);
        return false;
    }
    g_Skybox.Load(g_Map.GetSkyName());

    // --- player_start'tan spawn ---
    bool foundStart = false;
    for (const Entity& ent : g_Map.GetEntities()) {
        if (ent.Is(EntityClassnames::PlayerStart)) {
            if (const std::string* originStr = ent.Get(EntityKeys::Origin)) {
                glm::vec3 spawnPos = BSPMap::ParseOriginToEngineSpace(*originStr);
                spawnPos.y += 36.0f; // player_start origin genelde ayak hizasinda; goz hizasina tasi
                g_Camera.position = spawnPos;
                foundStart = true;
            }

            if (const std::string* angleStr = ent.Get(EntityKeys::Angle)) {
                float angle = static_cast<float>(std::atof(angleStr->c_str()));
                g_Camera.yaw = angle;
            }
            break;
        }
    }

    if (!foundStart) {
        Logger::warning("player_start bulunamadi, varsayilan konumdan spawn ediliyor.");
    }

    // crouch/velocity gibi player state'ini yeni haritaya gore sifirla
    g_Camera.verticalVelocity = 0.0f;
    g_Camera.isCrouching = false;
    g_Camera.onGround = false;

    return true;
}