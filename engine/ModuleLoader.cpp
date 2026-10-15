#include "ModuleLoader.h"
#include "console/Console.h"
#include <windows.h>

namespace {
    struct GameModule {
        HMODULE handle = nullptr;
        CreateInterfaceFn factory = nullptr;
    };

    GameModule s_clientModule;
    GameModule s_serverModule;
    IBaseClientDLL* s_clientDll = nullptr;
    IServerGameDLL* s_serverDll = nullptr;

    GameModule LoadModule(const std::string& relativePath) {
        GameModule module;

        // Goreli yol, calisma dizinine gore tam yola cevrilir; boylece
        // LoadLibrary'nin arama sirasindan etkilenilmez.
        char fullPath[MAX_PATH] = {};
        if (GetFullPathNameA(relativePath.c_str(), MAX_PATH, fullPath, nullptr) == 0) return module;

        module.handle = LoadLibraryA(fullPath);
        if (module.handle == nullptr) {
            Console::Log("Modul yuklenemedi: " + std::string(fullPath));
            return module;
        }

        module.factory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module.handle, "CreateInterface"));
        if (module.factory == nullptr) {
            Console::Log("Modulde CreateInterface bulunamadi: " + std::string(fullPath));
            FreeLibrary(module.handle);
            module.handle = nullptr;
        }
        return module;
    }
}

bool LoadGameModules(const std::string& gameDir) {
    bool ok = true;

    s_clientModule = LoadModule(gameDir + "/bin/client.dll");
    if (s_clientModule.factory != nullptr) {
        s_clientDll = GetInterface<IBaseClientDLL>(s_clientModule.factory, CLIENT_DLL_INTERFACE_VERSION);
        // Engine'in kendi factory'si: engine arayuzlerini DLL'e bu sunar.
        if (s_clientDll == nullptr || !s_clientDll->Init(&CreateInterface)) {
            Console::Log("client.dll Init basarisiz");
            s_clientDll = nullptr;
            ok = false;
        }
    }
    else ok = false;

    s_serverModule = LoadModule(gameDir + "/bin/server.dll");
    if (s_serverModule.factory != nullptr) {
        s_serverDll = GetInterface<IServerGameDLL>(s_serverModule.factory, SERVER_GAME_DLL_INTERFACE_VERSION);
        if (s_serverDll == nullptr || !s_serverDll->DLLInit(&CreateInterface)) {
            Console::Log("server.dll DLLInit basarisiz");
            s_serverDll = nullptr;
            ok = false;
        }
    }
    else ok = false;

    return ok;
}

void UnloadGameModules() {
    if (s_clientDll != nullptr) { s_clientDll->Shutdown(); s_clientDll = nullptr; }
    if (s_serverDll != nullptr) { s_serverDll->DLLShutdown(); s_serverDll = nullptr; }

    if (s_clientModule.handle != nullptr) { FreeLibrary(s_clientModule.handle); s_clientModule = GameModule(); }
    if (s_serverModule.handle != nullptr) { FreeLibrary(s_serverModule.handle); s_serverModule = GameModule(); }
}

IBaseClientDLL* GetClientDLL() { return s_clientDll; }
IServerGameDLL* GetServerDLL() { return s_serverDll; }