#include "pch.h"
#include "iservergamedll.h"
#include "ienginecommon.h"

// server.dll'in Faz 1 iskeleti. Hem oyun exe'si (tek oyunculu / listen server)
// hem dedicated server tarafindan yuklenir; bu yuzden host'a ozgu hicbir sey
// bilmez, yalnizca arayuzleri kullanir.
namespace {
    IEngineConsole* s_console = nullptr;
    IEngineServer* s_engine = nullptr;

    void LogLine(const char* message) {
        if (s_console != nullptr) s_console->Print(message);
    }
}

class ServerGameDLL : public IServerGameDLL {
public:
    bool DLLInit(CreateInterfaceFn engineFactory) override {
        s_console = GetInterface<IEngineConsole>(engineFactory, ENGINE_CONSOLE_INTERFACE_VERSION);
        s_engine = GetInterface<IEngineServer>(engineFactory, ENGINE_SERVER_INTERFACE_VERSION);

        if (s_console == nullptr) return false;

        LogLine("server.dll: DLLInit basarili");
        if (s_engine == nullptr) {
            LogLine("server.dll: IEngineServer henuz engine tarafindan saglanmiyor (Faz 1)");
        }
        return true;
    }

    void DLLShutdown() override {
        LogLine("server.dll: DLLShutdown");
        s_console = nullptr;
        s_engine = nullptr;
    }

    bool LevelInit(const char* mapName) override {
        LogLine((std::string("server.dll: LevelInit ") + (mapName ? mapName : "")).c_str());
        return true;
    }
    void LevelShutdown() override { LogLine("server.dll: LevelShutdown"); }

    void GameFrame(float) override {}

    bool ClientConnect(int, const char*) override { return true; }
    void ClientPutInServer(int) override {}
    void ClientDisconnect(int) override {}

    void RunPlayerCommand(int, const PlayerInputCommand&, float) override {}
    bool GetPlayerState(int, PlayerPhysicsState*) override { return false; }

    int GetEntityStates(NetEntityState*, int) override { return 0; }
};

EXPOSE_SINGLE_INTERFACE(ServerGameDLL, IServerGameDLL, SERVER_GAME_DLL_INTERFACE_VERSION);