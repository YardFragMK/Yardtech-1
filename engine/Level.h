#pragma once
#include <string>

// Harita yukleme/bosaltma islemini tek noktada toplar: VFS'ten okur, dunya
// verisini yukler, ardindan server.dll ve client.dll'e LevelInit bildirir.
// runLocalServer false ise (uzak bir sunucuya bagliyken) yerel server.dll
// calistirilmaz; yalnizca dunya ve client tarafi yuklenir.
bool LoadLevel(const std::string& mapName, bool runLocalServer = true);
void UnloadLevel();
const std::string& GetCurrentLevelName();