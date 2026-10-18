#include "interface.h"
#include "ienginecommon.h"
#include "ConVarRegistry.h"
#include "console/Console.h"

// Engine'in DLL'lere sundugu konsol arayuzu. Komut ve degisken kaydi
// ConVarRegistry'de tutulur; komut satirlari konsolun kendi ayristiricisindan gecer.
class EngineConsoleImpl : public IEngineConsole {
public:
    void Print(const char* message) override {
        Console::Log(message != nullptr ? message : "");
    }

    void Execute(const char* commandLine) override {
        if (commandLine != nullptr) Console::ExecuteLine(commandLine);
    }

    void RegisterCommand(const char* name, ConCommandFn fn, const char* help, int flags) override {
        ConVarRegistry::RegisterCommand(name, fn, help, flags);
    }

    float* RegisterFloat(const char* name, float defaultValue, const char* help, int flags) override {
        return ConVarRegistry::RegisterFloat(name, defaultValue, help, flags);
    }

    int* RegisterInt(const char* name, int defaultValue, const char* help, int flags) override {
        return ConVarRegistry::RegisterInt(name, defaultValue, help, flags);
    }
};

EXPOSE_SINGLE_INTERFACE(EngineConsoleImpl, IEngineConsole, ENGINE_CONSOLE_INTERFACE_VERSION);