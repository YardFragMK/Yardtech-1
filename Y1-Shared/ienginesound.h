#pragma once
#include <cstdint>
#include "interface.h"

#define ENGINE_SOUND_INTERFACE_VERSION "EngineSound003"

enum SoundChannel : int {
    SOUND_CHANNEL_SFX = 0,     // efektler: ana ses seviyesine bagli, oda efektinden etkilenir
    SOUND_CHANNEL_MUSIC = 1    // muzik: muzik ses seviyesine bagli, oda efektinden etkilenmez
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

    // Genisletilmis cagrilar: pitch 1.0 normaldir (0.25..4.0). 3B'de ses minDistance'a kadar tam
    // siddette, maxDistance'ta TAMAMEN sessizdir (dogrusal sonumleme).
    virtual SoundHandle Play2DEx(const char* vfsPath, float volume, int channel, bool loop, float pitch) = 0;
    virtual SoundHandle Play3DEx(const char* vfsPath, float x, float y, float z, float volume, bool loop,
        float pitch, float minDistance, float maxDistance) = 0;

    // Calan bir sesin ozellikleri (gecersiz tutamaclar yok sayilir).
    virtual void SetPosition(SoundHandle handle, float x, float y, float z) = 0;
    virtual void SetVolume(SoundHandle handle, float volume) = 0;
    virtual void SetPitch(SoundHandle handle, float pitch) = 0;
    virtual bool IsPlaying(SoundHandle handle) = 0;   // false: bitti, durduruldu ya da gecersiz
    virtual void Stop(SoundHandle handle) = 0;
    virtual void StopAll() = 0;

    // Dinleyici (kamera). Her karede cagrilmalidir; global ses seviyeleri de burada uygulanir.
    virtual void SetListener(float x, float y, float z, float forwardX, float forwardY, float forwardZ) = 0;

    // Oda efekti (Half-Life "roomtype" 0..28; 0 = yok). Efekt sesleri yumusak gecisle
    // boguklasir ve yankilanir.
    virtual void SetEnvironment(int roomType) = 0;
};