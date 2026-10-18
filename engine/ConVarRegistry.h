#pragma once
#include <string>
#include "ienginecommon.h"

// DLL'lerin IEngineConsole uzerinden kaydettigi komut ve degiskenlerin deposu.
// Konsol, kendi sabit komutlarini bulamazsa satiri buraya yonlendirir. Kayit
// bayraklari (tek oyunculu, hile, gelistirici) ayarlama aninda burada uygulanir.
namespace ConVarRegistry {
    void RegisterCommand(const char* name, ConCommandFn fn, const char* help, int flags);
    float* RegisterFloat(const char* name, float defaultValue, const char* help, int flags);
    int* RegisterInt(const char* name, int defaultValue, const char* help, int flags);

    // Kayitli degiskenin pointer'ini dondurur; yoksa veya turu farkliysa nullptr.
    float* FindFloat(const char* name);
    int* FindInt(const char* name);

    // Konsol satirini ayristirir. Kayitli bir komut/degisken bulunduysa true doner
    // (hata olsa bile); bulunamazsa false doner.
    bool TryExecute(const std::string& commandLine);

    void PrintAll();
}