#include "pch.h"
#include "iclientdll.h"
#include "ienginecommon.h"

// client.dll'in Faz 1 iskeleti: engine ile arayuz uzerinden konustugunu
// dogrulayan, henuz oyun mantigi icermeyen minimal gerceklestirme.
namespace {
    IEngineConsole* s_console = nullptr;
    IEngineClient* s_engine = nullptr;

    void LogLine(const char* message) {
        if (s_console != nullptr) s_console->Print(message);
    }
}

class ClientDLL : public IBaseClientDLL {
public:
    bool Init(CreateInterfaceFn engineFactory) override {
        s_console = GetInterface<IEngineConsole>(engineFactory, ENGINE_CONSOLE_INTERFACE_VERSION);
        s_engine = GetInterface<IEngineClient>(engineFactory, ENGINE_CLIENT_INTERFACE_VERSION);

        if (s_console == nullptr) return false;

        LogLine("client.dll: Init basarili");
        if (s_engine == nullptr) {
            LogLine("client.dll: IEngineClient henuz engine tarafindan saglanmiyor (Faz 1)");
        }
        return true;
    }

    void Shutdown() override {
        LogLine("client.dll: Shutdown");
        s_console = nullptr;
        s_engine = nullptr;
    }

    void LevelInit(const char* mapName) override {
        LogLine((std::string("client.dll: LevelInit ") + (mapName ? mapName : "")).c_str());
    }
    void LevelShutdown() override { LogLine("client.dll: LevelShutdown"); }

    void OnButtonEvent(int, bool) override {}
    void OnMouseMove(float, float, float, float) override {}
    void OnMouseWheel(float) override {}
    void OnTextInput(unsigned int) override {}

    void CreateMove(float, PlayerInputCommand* outCommand) override {
        if (outCommand != nullptr) *outCommand = PlayerInputCommand();
    }

    void Update(float) override {}
    void GetView(ViewSetup* outView) override {
        if (outView != nullptr) *outView = ViewSetup();
    }

    void ReceiveEntityStates(const NetEntityState*, int) override {}
    void ReceivePlayerState(const PlayerPhysicsState&, unsigned int) override {}

    void RenderScene() override {}
    void RenderUI(int, int) override {}
};

EXPOSE_SINGLE_INTERFACE(ClientDLL, IBaseClientDLL, CLIENT_DLL_INTERFACE_VERSION);