#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "ienginesound.h"
#include "Engine.h"
#include "console/CVar.h"
#include "console/Console.h"
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// miniaudio uzerine kurulu ses hizmeti. Ses cihazi ilk kullanimda acilir; hic ses
// calinmayan oturumlarda cihaz acilmaz. Kodlanmis ses verisi bellekte tutulur ve
// miniaudio'nun kaynak yoneticisine adla kaydedilir; boylece dosyalar VFS'ten
// (paket ya da klasor) gelebilir.
namespace {
    constexpr int MAX_ACTIVE_SOUNDS = 64;   // asiri ses uretimine karsi ust sinir
    constexpr float MIN_DISTANCE = 128.0f;  // bu mesafeye kadar sonumleme yok
    constexpr float MAX_DISTANCE = 4000.0f; // bundan uzakta ses artik azalmaz/duyulmaz

    struct ActiveSound {
        ma_sound sound;
        SoundHandle handle = 0;
    };

    ma_engine s_engine;
    ma_sound_group s_groups[2]; // SoundChannel degerleriyle indekslenir
    bool s_initialized = false;
    bool s_unavailable = false; // baslatma basarisiz oldu ya da kapatildi: tekrar denenmez
    SoundHandle s_nextHandle = 1;

    std::vector<std::unique_ptr<ActiveSound>> s_active;
    std::unordered_map<std::string, std::vector<uint8_t>> s_encoded;

    float Clamp(float value, float low, float high) {
        return value < low ? low : (value > high ? high : value);
    }

    void ApplyVolumes() {
        ma_engine_set_volume(&s_engine, Clamp(g_CVar.nvs_mastervolume, 0.0f, 1.0f));
        ma_sound_group_set_volume(&s_groups[SOUND_CHANNEL_MUSIC], Clamp(g_CVar.nvs_musicvolume, 0.0f, 1.0f));
    }

    void Shutdown() {
        if (!s_initialized) return;

        for (auto& active : s_active) ma_sound_uninit(&active->sound);
        s_active.clear();

        for (int i = 0; i < 2; i++) ma_sound_group_uninit(&s_groups[i]);
        ma_engine_uninit(&s_engine);

        // Kaynak yoneticisi kapandiktan sonra bellek serbest birakilabilir.
        s_encoded.clear();

        s_initialized = false;
        s_unavailable = true;
    }

    bool EnsureInit() {
        if (s_initialized) return true;
        if (s_unavailable) return false;

        ma_engine_config config = ma_engine_config_init();
        if (ma_engine_init(&config, &s_engine) != MA_SUCCESS) {
            s_unavailable = true;
            Console::Log("Ses motoru baslatilamadi (ses cihazi bulunamadi mi?). Ses devre disi.");
            return false;
        }

        for (int i = 0; i < 2; i++) ma_sound_group_init(&s_engine, 0, nullptr, &s_groups[i]);
        ma_engine_listener_set_world_up(&s_engine, 0, 0.0f, 1.0f, 0.0f); // motor Y-up
        ApplyVolumes();

        s_initialized = true;

        // Surec kapanirken, statik nesneler yok edilmeden once cihaz duzgunce kapatilir.
        std::atexit(&Shutdown);
        return true;
    }

    bool EnsureLoaded(const std::string& path) {
        if (s_encoded.count(path) != 0) return true;

        std::vector<uint8_t> data = vfs.ReadFile(path);
        if (data.empty()) {
            Console::Log("Ses dosyasi bulunamadi: " + path);
            return false;
        }

        auto inserted = s_encoded.emplace(path, std::move(data));
        const std::vector<uint8_t>& stored = inserted.first->second;

        if (ma_resource_manager_register_encoded_data(ma_engine_get_resource_manager(&s_engine),
            path.c_str(), stored.data(), stored.size()) != MA_SUCCESS) {
            s_encoded.erase(inserted.first);
            return false;
        }
        return true;
    }

    // Bitmis sesler listeden kaldirilir (dongulu sesler bitmez).
    void ReclaimFinished() {
        for (size_t i = 0; i < s_active.size();) {
            if (ma_sound_at_end(&s_active[i]->sound)) {
                ma_sound_uninit(&s_active[i]->sound);
                s_active.erase(s_active.begin() + static_cast<std::ptrdiff_t>(i));
            }
            else {
                ++i;
            }
        }
    }

    ActiveSound* FindSound(SoundHandle handle) {
        if (handle == 0) return nullptr;
        for (auto& active : s_active) {
            if (active->handle == handle) return active.get();
        }
        return nullptr;
    }

    void RemoveSound(SoundHandle handle) {
        for (size_t i = 0; i < s_active.size(); i++) {
            if (s_active[i]->handle == handle) {
                ma_sound_uninit(&s_active[i]->sound);
                s_active.erase(s_active.begin() + static_cast<std::ptrdiff_t>(i));
                return;
            }
        }
    }

    ActiveSound* CreateSound(const std::string& path, int channel, bool spatial, bool loop, float volume) {
        ReclaimFinished();
        if (s_active.size() >= static_cast<size_t>(MAX_ACTIVE_SOUNDS)) return nullptr;
        if (!EnsureInit() || !EnsureLoaded(path)) return nullptr;

        const int channelIndex = (channel == SOUND_CHANNEL_MUSIC) ? SOUND_CHANNEL_MUSIC : SOUND_CHANNEL_SFX;

        // Kisa efektler tamamen cozulup bellekte tutulur, muzik akis olarak cozulur.
        ma_uint32 flags = (channelIndex == SOUND_CHANNEL_MUSIC) ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
        if (!spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

        auto active = std::make_unique<ActiveSound>();
        if (ma_sound_init_from_file(&s_engine, path.c_str(), flags, &s_groups[channelIndex], nullptr, &active->sound) != MA_SUCCESS) {
            Console::Log("Ses calinamadi (desteklenmeyen bicim olabilir): " + path);
            return nullptr;
        }

        ma_sound_set_volume(&active->sound, Clamp(volume, 0.0f, 4.0f));
        ma_sound_set_looping(&active->sound, loop ? MA_TRUE : MA_FALSE);

        if (spatial) {
            ma_sound_set_attenuation_model(&active->sound, ma_attenuation_model_inverse);
            ma_sound_set_min_distance(&active->sound, MIN_DISTANCE);
            ma_sound_set_max_distance(&active->sound, MAX_DISTANCE);
            ma_sound_set_rolloff(&active->sound, 1.0f);
        }

        active->handle = s_nextHandle++;
        s_active.push_back(std::move(active));
        return s_active.back().get();
    }
}

class EngineSoundImpl : public IEngineSound {
public:
    bool Precache(const char* vfsPath) override {
        if (vfsPath == nullptr || !EnsureInit()) return false;
        return EnsureLoaded(vfsPath);
    }

    SoundHandle Play2D(const char* vfsPath, float volume, int channel, bool loop) override {
        if (vfsPath == nullptr) return 0;

        ActiveSound* active = CreateSound(vfsPath, channel, false, loop, volume);
        if (active == nullptr) return 0;

        ma_sound_start(&active->sound);
        return active->handle;
    }

    SoundHandle Play3D(const char* vfsPath, float x, float y, float z, float volume, bool loop) override {
        if (vfsPath == nullptr) return 0;

        ActiveSound* active = CreateSound(vfsPath, SOUND_CHANNEL_SFX, true, loop, volume);
        if (active == nullptr) return 0;

        ma_sound_set_position(&active->sound, x, y, z);
        ma_sound_start(&active->sound);
        return active->handle;
    }

    void SetPosition(SoundHandle handle, float x, float y, float z) override {
        if (ActiveSound* active = FindSound(handle)) {
            ma_sound_set_position(&active->sound, x, y, z);
        }
    }

    void Stop(SoundHandle handle) override {
        RemoveSound(handle);
    }

    void StopAll() override {
        for (auto& active : s_active) ma_sound_uninit(&active->sound);
        s_active.clear();
    }

    void SetListener(float x, float y, float z, float forwardX, float forwardY, float forwardZ) override {
        // Hic ses calinmadiysa cihaz acik degildir; yapilacak is yok.
        if (!s_initialized) return;

        ma_engine_listener_set_position(&s_engine, 0, x, y, z);
        ma_engine_listener_set_direction(&s_engine, 0, forwardX, forwardY, forwardZ);
        ApplyVolumes(); // Ayarlar'daki kaydiricilar cvar'lari degistirir, burada uygulanir
        ReclaimFinished();
    }
};

EXPOSE_SINGLE_INTERFACE(EngineSoundImpl, IEngineSound, ENGINE_SOUND_INTERFACE_VERSION);