#include "interface.h"
#include "ienginecommon.h"
#include "console/Console.h"
#include <string>
#include <memory>
#include <unordered_map>

// Engine'in DLL'lere sundugu arayuzlerin gerceklestirmeleri. Faz 1'de yalnizca
// IEngineConsole vardir; IEngineClient/IEngineServer Faz 2'de eklenecektir.
class EngineConsoleImpl : public IEngineConsole {
public:
    void Print(const char* message) override {
        Console::Log(message != nullptr ? message : "");
    }

    void Execute(const char* commandLine) override {
        // Konsolun komut ayristiricisina baglanma Faz 2'de yapilacak.
        Console::Log(std::string("Execute henuz baglanmadi: ") + (commandLine != nullptr ? commandLine : ""));
    }

    void RegisterCommand(const char* name, ConCommandFn fn, const char* help, int flags) override {
        m_commands[name] = CommandEntry{ fn, help != nullptr ? help : "", flags };
    }

    // Degisken pointer'lari surec boyunca sabit kalmalidir; bu yuzden degerler
    // unique_ptr ile ayri ayri ayrilir (map yeniden duzenlense de adres degismez).
    float* RegisterFloat(const char* name, float defaultValue, const char*, int) override {
        auto& slot = m_floats[name];
        if (!slot) slot = std::make_unique<float>(defaultValue);
        return slot.get();
    }

    int* RegisterInt(const char* name, int defaultValue, const char*, int) override {
        auto& slot = m_ints[name];
        if (!slot) slot = std::make_unique<int>(defaultValue);
        return slot.get();
    }

private:
    struct CommandEntry {
        ConCommandFn fn = nullptr;
        std::string help;
        int flags = 0;
    };

    std::unordered_map<std::string, CommandEntry> m_commands;
    std::unordered_map<std::string, std::unique_ptr<float>> m_floats;
    std::unordered_map<std::string, std::unique_ptr<int>> m_ints;
};

EXPOSE_SINGLE_INTERFACE(EngineConsoleImpl, IEngineConsole, ENGINE_CONSOLE_INTERFACE_VERSION);