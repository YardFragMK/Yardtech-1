#pragma once

#define ENGINE_CONSOLE_INTERFACE_VERSION "EngineConsole001"

enum ConVarFlags : int {
    CVAR_NONE = 0,
    CVAR_SINGLEPLAYER_ONLY = 1 << 0, // sunucuya bagliyken reddedilir
    CVAR_SERVER_ONLY = 1 << 1,       // yalnizca dedicated server konsolundan
    CVAR_CLIENT_ONLY = 1 << 2,
    CVAR_CHEAT = 1 << 3,             // nvs_cheats kapaliysa reddedilir
    CVAR_ARCHIVE = 1 << 4,           // config dosyasina kaydedilir
    CVAR_DEVELOPER = 1 << 5          // nvs_developer kapaliysa reddedilir
};

typedef void (*ConCommandFn)(int argc, const char* const* argv);

// Hem client hem server tarafindaki engine bu arayuzu sunar. DLL'ler komut ve
// degisken kaydini buradan yapar; degisken pointer'lari engine'e aittir ve
// surec boyunca sabit kalir.
class IEngineConsole {
public:
    virtual void Print(const char* message) = 0;
    virtual void Execute(const char* commandLine) = 0;
    virtual void RegisterCommand(const char* name, ConCommandFn fn, const char* help, int flags) = 0;
    virtual float* RegisterFloat(const char* name, float defaultValue, const char* help, int flags) = 0;
    virtual int* RegisterInt(const char* name, int defaultValue, const char* help, int flags) = 0;
};