#include "pch.h"
#include <string>
#include "iservergamedll.h"
#include "ienginecommon.h"
#include "ServerContext.h"
#include "BotManager.h"

// server.dll'in gercek gerceklestirmesi. Hem oyun exe'si (tek oyunculu/listen
// server) hem dedicated server tarafindan yuklenir; host'a ozgu hicbir sey
// bilmez, yalnizca arayuzleri kullanir.
class ServerGameDLL : public IServerGameDLL {
public:
    bool DLLInit(CreateInterfaceFn engineFactory) override {
        ServerContext& ctx = Ctx();
        ctx.console = GetInterface<IEngineConsole>(engineFactory, ENGINE_CONSOLE_INTERFACE_VERSION);
        ctx.engine = GetInterface<IEngineServer>(engineFactory, ENGINE_SERVER_INTERFACE_VERSION);

        if (ctx.console == nullptr) return false;

        ctx.Log("server.dll: DLLInit basarili");
        if (ctx.engine == nullptr) ctx.Log("server.dll: IEngineServer saglanamadi");
        return true;
    }

    void DLLShutdown() override {
        ServerContext& ctx = Ctx();
        ctx.Log("server.dll: DLLShutdown");
        BotManager::Shutdown();
        ctx.world = nullptr;
        ctx.engine = nullptr;
        ctx.console = nullptr;
    }

    bool LevelInit(const char* mapName) override {
        ServerContext& ctx = Ctx();

        ctx.world = (ctx.engine != nullptr) ? ctx.engine->GetWorld() : nullptr;
        if (ctx.world == nullptr) {
            ctx.Log("server.dll: dunya sorgu arayuzu yok, LevelInit basarisiz");
            return false;
        }

        for (PlayerSlot& slot : ctx.slots) slot = PlayerSlot();

        BotManager::Init();
        ctx.Log((std::string("server.dll: LevelInit ") + (mapName ? mapName : "")).c_str());
        return true;
    }

    void LevelShutdown() override {
        ServerContext& ctx = Ctx();
        BotManager::Shutdown();
        for (PlayerSlot& slot : ctx.slots) slot = PlayerSlot();
        ctx.world = nullptr;
        ctx.Log("server.dll: LevelShutdown");
    }

    void GameFrame(float frameTime) override {
        for (PlayerSlot& slot : Ctx().slots) {
            if (slot.connected) slot.player.Update(frameTime);
        }
        BotManager::Update(frameTime);
    }

    bool ClientConnect(int clientIndex, const char*) override {
        PlayerSlot* slot = Ctx().GetSlot(clientIndex);
        if (slot == nullptr) return false;

        *slot = PlayerSlot();
        slot->connected = true;
        return true;
    }

    void ClientPutInServer(int) override {}

    void ClientDisconnect(int clientIndex) override {
        if (PlayerSlot* slot = Ctx().GetSlot(clientIndex)) *slot = PlayerSlot();
    }

    // Oyuncu hareketi Faz 3'te server'a tasinacak.
    void RunPlayerCommand(int, const PlayerInputCommand&, float) override {}
    bool GetPlayerState(int, PlayerPhysicsState*) override { return false; }

    void SetPlayerView(int clientIndex, const glm::vec3& eyePosition, float moveSpeed) override {
        PlayerSlot* slot = Ctx().GetSlot(clientIndex);
        if (slot == nullptr || !slot->connected) return;

        slot->eyePosition = eyePosition;
        slot->moveSpeed = moveSpeed;
    }

    void RunPlayerActions(int clientIndex, const PlayerActionCommand& actions) override {
        PlayerSlot* slot = Ctx().GetSlot(clientIndex);
        if (slot == nullptr || !slot->connected) return;

        slot->player.ApplyActions(actions);
    }

    bool GetPlayerStats(int clientIndex, PlayerStats* outStats) override {
        PlayerSlot* slot = Ctx().GetSlot(clientIndex);
        if (slot == nullptr || !slot->connected || outStats == nullptr) return false;

        *outStats = slot->player.GetStats();
        return true;
    }

    int GetEntityStates(NetEntityState* outStates, int maxCount) override {
        if (outStates == nullptr || maxCount <= 0) return 0;
        return BotManager::CollectEntityStates(outStates, maxCount);
    }
};

EXPOSE_SINGLE_INTERFACE(ServerGameDLL, IServerGameDLL, SERVER_GAME_DLL_INTERFACE_VERSION);