#pragma once
#include <string>

// Harita yukleme/bosaltma islemini tek noktada toplar: VFS'ten okur, dunya
// verisini yukler, ardindan server.dll ve client.dll'e LevelInit bildirir.
bool LoadLevel(const std::string& mapName);
void UnloadLevel();
const std::string& GetCurrentLevelName();