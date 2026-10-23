#pragma once
#include <cstdint>
#include "interface.h"

#define ENGINE_SOUND_INTERFACE_VERSION "EngineSound001"

enum SoundChannel : int {
    SOUND_CHANNEL_SFX = 0,
    SOUND_CHANNEL_MUSIC = 1
};

// 0 gecersiz tutamactir. Biten sesin tutamaci gecersiz hale gelir; gecersiz
// tutamaclarla yapilan cagrilar zararsizca yok sayilir.
typedef uint32_t SoundHandle;

// Engine'in modullere (client.dll, ileride server.dll) sundugu ses hizmeti.
// Yollar VFS'e goredir (orn. "sound/pistol.wav"). Konumlar dunya birimindedir.
class IEngineSound {
public:
    // Dosyayi onceden yukler; Play cagrilari gerekirse kendisi de yukler.
    virtual bool Precache(const char* vfsPath) = 0;

    // Konumsuz (2B) ses: arayuz, silah, muzik. volume 0..4 araligina kirpilir.
    virtual SoundHandle Play2D(const char* vfsPath, float volume, int channel, bool loop) = 0;

    // Dunya konumunda (3B) ses: dinleyiciye uzakliga gore sonumlenir ve saga/sola dagilir.
    virtual SoundHandle Play3D(const char* vfsPath, float x, float y, float z, float volume, bool loop) = 0;

    virtual void SetPosition(SoundHandle handle, float x, float y, float z) = 0;
    virtual void Stop(SoundHandle handle) = 0;
    virtual void StopAll() = 0;

    // Dinleyici (kamera). Her karede cagrilmalidir; global ses seviyeleri de burada uygulanir.
    virtual void SetListener(float x, float y, float z, float forwardX, float forwardY, float forwardZ) = 0;
};