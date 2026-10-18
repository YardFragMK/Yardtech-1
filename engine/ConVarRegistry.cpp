#include "ConVarRegistry.h"
#include "NetClient.h"
#include "console/Console.h"
#include "console/CVar.h"
#include <cstdlib>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace {
    enum class VarType { Command, Float, Int };

    struct Entry {
        VarType type = VarType::Command;
        ConCommandFn command = nullptr;
        std::unique_ptr<float> floatValue;
        std::unique_ptr<int> intValue;
        std::string help;
        int flags = 0;

        bool IsEmpty() const { return command == nullptr && !floatValue && !intValue; }
    };

    // Degiskenlerin adresleri surec boyunca sabit kalmalidir (DLL'ler pointer'i
    // saklar); bu yuzden degerler unique_ptr ile ayri ayri ayrilir.
    std::map<std::string, Entry>& Entries() {
        static std::map<std::string, Entry> entries;
        return entries;
    }

    bool CheckFlags(const std::string& name, int flags) {
        if ((flags & CVAR_SERVER_ONLY) != 0) {
            Console::Log(name + ": bu komut yalnizca dedicated server konsolundan calisir");
            return false;
        }
        if ((flags & CVAR_SINGLEPLAYER_ONLY) != 0 && NetClient::IsConnected()) {
            Console::Log(name + ": bu komut sadece singleplayer modda calisir (sunucuya bagliyken degil)");
            return false;
        }
        if ((flags & CVAR_CHEAT) != 0 && !g_CVar.nvs_cheats) {
            Console::Log(name + ": cheats required (nvs_cheats 1)");
            return false;
        }
        if ((flags & CVAR_DEVELOPER) != 0 && g_CVar.nvs_developer != 1) {
            Console::Log(name + ": developer mode required");
            return false;
        }
        return true;
    }
}

void ConVarRegistry::RegisterCommand(const char* name, ConCommandFn fn, const char* help, int flags) {
    if (name == nullptr || fn == nullptr) return;

    Entry& entry = Entries()[name];
    entry.type = VarType::Command;
    entry.command = fn;
    entry.help = help != nullptr ? help : "";
    entry.flags = flags;
}

float* ConVarRegistry::RegisterFloat(const char* name, float defaultValue, const char* help, int flags) {
    if (name == nullptr) return nullptr;

    Entry& entry = Entries()[name];
    if (entry.type == VarType::Float && entry.floatValue) return entry.floatValue.get();

    if (!entry.IsEmpty()) {
        Console::Log(std::string(name) + ": farkli turde zaten kayitli");
        return nullptr;
    }

    entry.type = VarType::Float;
    entry.floatValue = std::make_unique<float>(defaultValue);
    entry.help = help != nullptr ? help : "";
    entry.flags = flags;
    return entry.floatValue.get();
}

int* ConVarRegistry::RegisterInt(const char* name, int defaultValue, const char* help, int flags) {
    if (name == nullptr) return nullptr;

    Entry& entry = Entries()[name];
    if (entry.type == VarType::Int && entry.intValue) return entry.intValue.get();

    if (!entry.IsEmpty()) {
        Console::Log(std::string(name) + ": farkli turde zaten kayitli");
        return nullptr;
    }

    entry.type = VarType::Int;
    entry.intValue = std::make_unique<int>(defaultValue);
    entry.help = help != nullptr ? help : "";
    entry.flags = flags;
    return entry.intValue.get();
}

float* ConVarRegistry::FindFloat(const char* name) {
    if (name == nullptr) return nullptr;
    auto it = Entries().find(name);
    if (it == Entries().end() || it->second.type != VarType::Float) return nullptr;
    return it->second.floatValue.get();
}

int* ConVarRegistry::FindInt(const char* name) {
    if (name == nullptr) return nullptr;
    auto it = Entries().find(name);
    if (it == Entries().end() || it->second.type != VarType::Int) return nullptr;
    return it->second.intValue.get();
}

bool ConVarRegistry::TryExecute(const std::string& commandLine) {
    std::istringstream stream(commandLine);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) tokens.push_back(token);

    if (tokens.empty()) return false;

    auto it = Entries().find(tokens[0]);
    if (it == Entries().end()) return false;

    const std::string& name = tokens[0];
    Entry& entry = it->second;

    switch (entry.type) {
    case VarType::Command: {
        if (!CheckFlags(name, entry.flags)) return true;

        std::vector<const char*> argv;
        for (const std::string& t : tokens) argv.push_back(t.c_str());
        entry.command(static_cast<int>(argv.size()), argv.data());
        return true;
    }
    case VarType::Float: {
        if (tokens.size() < 2) {
            Console::Log(name + " = " + std::to_string(*entry.floatValue) +
                (entry.help.empty() ? "" : "  (" + entry.help + ")"));
            return true;
        }
        if (!CheckFlags(name, entry.flags)) return true;

        char* end = nullptr;
        float value = std::strtof(tokens[1].c_str(), &end);
        if (end == tokens[1].c_str() || *end != '\0') {
            Console::Log("Usage: " + name + " <value>");
            return true;
        }

        *entry.floatValue = value;
        Console::Log(name + " set to " + std::to_string(value));
        return true;
    }
    case VarType::Int: {
        if (tokens.size() < 2) {
            Console::Log(name + " = " + std::to_string(*entry.intValue) +
                (entry.help.empty() ? "" : "  (" + entry.help + ")"));
            return true;
        }
        if (!CheckFlags(name, entry.flags)) return true;

        char* end = nullptr;
        long value = std::strtol(tokens[1].c_str(), &end, 10);
        if (end == tokens[1].c_str() || *end != '\0') {
            Console::Log("Usage: " + name + " <integer>");
            return true;
        }

        *entry.intValue = static_cast<int>(value);
        Console::Log(name + " set to " + std::to_string(value));
        return true;
    }
    }
    return false;
}

void ConVarRegistry::PrintAll() {
    for (const auto& pair : Entries()) {
        const std::string& name = pair.first;
        const Entry& entry = pair.second;

        if ((entry.flags & CVAR_DEVELOPER) != 0 && g_CVar.nvs_developer != 1) continue;

        switch (entry.type) {
        case VarType::Float:   Console::Log(name + " " + std::to_string(*entry.floatValue)); break;
        case VarType::Int:     Console::Log(name + " " + std::to_string(*entry.intValue)); break;
        case VarType::Command: Console::Log(name + " (komut)"); break;
        }
    }
}