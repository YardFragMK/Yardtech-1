#include "ShaderLibrary.h"
#include "../ConVarRegistry.h"
#include "../console/Console.h"
#include <unordered_map>

namespace {
    struct Entry {
        std::string vertPath;
        std::string fragPath;
        ShaderProgram program;
    };

    // Dugum tabanli kapsayici: Entry adresleri ekleme/silme ile degismez.
    std::unordered_map<std::string, Entry>& Entries() {
        static std::unordered_map<std::string, Entry> entries;
        return entries;
    }

    bool s_commandsRegistered = false;
}

bool ShaderLibrary::Load(const std::string& name, const std::string& vertName) {
    Entry& entry = Entries()[name];
    entry.vertPath = "shader/" + (vertName.empty() ? name : vertName) + ".vert";
    entry.fragPath = "shader/" + name + ".frag";

    const bool ok = entry.program.Load(entry.vertPath, entry.fragPath);
    if (ok) Console::Log("Shader yuklendi: " + name);
    return ok;
}

ShaderProgram* ShaderLibrary::Get(const std::string& name) {
    auto it = Entries().find(name);
    if (it == Entries().end() || !it->second.program.IsValid()) return nullptr;
    return &it->second.program;
}

void ShaderLibrary::ReloadAll() {
    int loaded = 0;
    for (auto& pair : Entries()) {
        Entry& entry = pair.second;
        if (entry.program.Load(entry.vertPath, entry.fragPath)) ++loaded;
    }
    Console::Log("Shader'lar yeniden yuklendi: " + std::to_string(loaded) + "/" + std::to_string(Entries().size()));
}

void ShaderLibrary::RegisterCommands() {
    if (s_commandsRegistered) return;
    s_commandsRegistered = true;

    ConVarRegistry::RegisterCommand("shader_reload",
        [](int, const char* const*) { ShaderLibrary::ReloadAll(); },
        "Tum shader dosyalarini VFS'ten yeniden yukler", CVAR_NONE);
}