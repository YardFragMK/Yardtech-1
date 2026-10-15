#pragma once
#include <string>
#include "iclientdll.h"
#include "iservergamedll.h"

// gameDir/bin/client.dll ve server.dll dosyalarini yukler, arayuzlerini alir
// ve Init'lerini cagirir. Basarisiz olan modul nullptr olarak kalir.
bool LoadGameModules(const std::string& gameDir);
void UnloadGameModules();

IBaseClientDLL* GetClientDLL();
IServerGameDLL* GetServerDLL();