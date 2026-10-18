#include "MapLoader.h"
#include "BSPMap.h"
#include "BSPMapRenderer.h"
#include "Logger.h"
#include "Skybox.h"

bool LoadMap(const std::string& bspPath) {
    std::string fullPath = "nvs1/map/" + bspPath + ".bsp";

    if (!g_Map.Load(fullPath)) {
        Logger::error("BSP yuklenemedi (collision): " + bspPath);
        return false;
    }
    if (!g_MapRenderer.Load(fullPath, { "", "map/", "wads/", "textures/" })) {
        Logger::error("BSP yuklenemedi (render): " + bspPath);
        return false;
    }

    g_Skybox.Load(g_Map.GetSkyName());
    return true;
}

bool LoadMapFromMemory(const std::vector<uint8_t>& bspData, const std::string& mapName) {
    if (bspData.empty()) {
        Logger::error("BSP verisi bos, harita yuklenemedi: " + mapName);
        return false;
    }

    if (!g_Map.LoadFromMemory(bspData)) {
        Logger::error("BSP yuklenemedi (collision memory): " + mapName);
        return false;
    }
    if (!g_MapRenderer.LoadFromMemory(bspData)) {
        Logger::error("BSP yuklenemedi (render memory): " + mapName);
        return false;
    }

    g_Skybox.Load(g_Map.GetSkyName());
    return true;
}