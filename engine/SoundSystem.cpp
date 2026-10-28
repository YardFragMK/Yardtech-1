#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "ienginesound.h"
#include "SoundSystem.h"
#include "Engine.h"
#include "console/CVar.h"
#include "console/Console.h"
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// miniaudio uzerine kurulu ses hizmeti. Ses cihazi ilk kullanimda acilir; hic ses
// calinmayan oturumlarda cihaz acilmaz. Kodlanmis ses verisi bellekte tutulur ve miniaudio'nun
// kaynak yoneticisine adla kaydedilir; boylece dosyalar VFS'ten (paket ya da klasor) gelebilir.
namespace {

    //--------------------------------------------------------------------------
    // Sabitler
    //--------------------------------------------------------------------------
    constexpr int kMaxActiveSounds = 96;            // ayni anda en fazla ses (fazlasi sessizce reddedilir)
    constexpr float kDefaultMinDistance = 128.0f;   // Play3D icin: bu mesafeye kadar tam siddet
    constexpr float kDefaultMaxDistance = 4000.0f;  // Play3D icin: bundan uzakta ses azalmayi birakir
    constexpr float kMaxVolume = 4.0f;              // bir sesin alabilecegi en yuksek carpan
    constexpr float kMinPitch = 0.25f;              // en yavas (4 kat dusuk perde)
    constexpr float kMaxPitch = 4.0f;               // en hizli (4 kat yuksek perde)

    //--------------------------------------------------------------------------
    // Oda efekti: iki dugumlu bir zincir. SFX grubu -> alcak geciren filtre -> yanki -> cikis.
    //--------------------------------------------------------------------------
    // Yankinin gecikmesi calisirken degistirilemez; tum odalar icin sabit tutulur (saniye).
    constexpr float kEchoDelaySeconds = 0.07f;

    // Filtre ve yanki degerlerinin hedefe yaklasma hizi (saniyede hedefe yakinlasma orani).
    // 6 = yaklasik yarim saniyede yeni odaya gecis; ani tiklama olmaz.
    constexpr float kEnvironmentBlendPerSecond = 6.0f;

    // Alcak geciren filtre, kesme frekansi bu oranda degisince yeniden kurulur (%2).
    constexpr float kCutoffUpdateThreshold = 0.02f;

    // Su alti oda tipi: "water2" on ayari.
    constexpr int kUnderwaterRoomType = 15;

    // Bir odanin ses karakteri.
    struct RoomPreset {
        float cutoffHz;   // alcak geciren filtre: bunun ustundeki frekanslar kesilir (dusuk = bogugk)
        float wet;        // yanki seviyesi 0..1 (0 = yanki yok)
        float decay;      // yankinin kendini tekrarlama orani 0..1 (yuksek = uzun yankilanir)
    };

    // Half-Life "roomtype" degerleri (0..28). Siralama HL'deki gibidir; degerler yaklasiktir.
    constexpr int kRoomTypeCount = 29;
    const RoomPreset kRoomPresets[kRoomTypeCount] = {
        // cutoffHz  wet    decay
        { 20000.0f, 0.00f, 0.00f },   //  0 yok
        { 14000.0f, 0.18f, 0.25f },   //  1 genel
        { 16000.0f, 0.28f, 0.35f },   //  2 metal kucuk
        { 16000.0f, 0.33f, 0.40f },   //  3 metal orta
        { 15000.0f, 0.38f, 0.45f },   //  4 metal buyuk
        {  9000.0f, 0.35f, 0.50f },   //  5 tunel kucuk
        {  8500.0f, 0.40f, 0.55f },   //  6 tunel orta
        {  8000.0f, 0.45f, 0.60f },   //  7 tunel buyuk
        {  9000.0f, 0.30f, 0.45f },   //  8 oda kucuk
        {  8500.0f, 0.35f, 0.50f },   //  9 oda orta
        {  8000.0f, 0.40f, 0.55f },   // 10 oda buyuk
        { 18000.0f, 0.25f, 0.30f },   // 11 parlak kucuk
        { 18000.0f, 0.30f, 0.35f },   // 12 parlak orta
        { 17000.0f, 0.35f, 0.40f },   // 13 parlak buyuk
        {  1400.0f, 0.20f, 0.30f },   // 14 su 1 (boguk)
        {  1000.0f, 0.22f, 0.30f },   // 15 su 2
        {   700.0f, 0.25f, 0.30f },   // 16 su 3 (en boguk)
        {  8000.0f, 0.28f, 0.40f },   // 17 beton kucuk
        {  8000.0f, 0.33f, 0.45f },   // 18 beton orta
        {  7500.0f, 0.38f, 0.50f },   // 19 beton buyuk
        {  7000.0f, 0.35f, 0.50f },   // 20 genis 1
        {  6500.0f, 0.40f, 0.55f },   // 21 genis 2
        {  6000.0f, 0.45f, 0.60f },   // 22 genis 3
        {  5000.0f, 0.38f, 0.55f },   // 23 magara kucuk
        {  4500.0f, 0.42f, 0.60f },   // 24 magara orta
        {  4000.0f, 0.46f, 0.65f },   // 25 magara buyuk
        {  3000.0f, 0.45f, 0.65f },   // 26 garip 1
        {  2500.0f, 0.50f, 0.70f },   // 27 garip 2
        {  2000.0f, 0.55f, 0.75f }    // 28 garip 3
    };

    //--------------------------------------------------------------------------
    // Durum
    //--------------------------------------------------------------------------
    struct ActiveSound {
        ma_sound sound;
        SoundHandle handle = 0;
    };

    struct SoundParams {
        int channel = SOUND_CHANNEL_SFX;
        bool spatial = false;                 // true: konumlu (3B)
        bool loop = false;
        float volume = 1.0f;
        float pitch = 1.0f;
        float minDistance = kDefaultMinDistance;
        float maxDistance = kDefaultMaxDistance;
        bool linearFalloff = false;           // true: maxDistance'ta tamamen sessiz
    };

    ma_engine g_engine;
    ma_sound_group g_groups[2];               // SoundChannel degerleriyle indekslenir
    ma_lpf_node g_lowPassNode;                // oda efekti: alcak geciren filtre
    ma_delay_node g_echoNode;                 // oda efekti: yanki
    bool g_environmentReady = false;          // oda efekti dugumleri kuruldu mu

    bool g_initialized = false;
    bool g_unavailable = false;               // baslatma basarisiz oldu ya da kapatildi: tekrar denenmez
    SoundHandle g_nextHandle = 1;

    std::vector<std::unique_ptr<ActiveSound>> g_active;
    std::unordered_map<std::string, std::vector<uint8_t>> g_encoded;   // kayitli kodlanmis ses verisi
    std::unordered_set<std::string> g_failedPaths;                     // bir kez basarisiz olan yollar (tekrar denenmez)

    // Oda efekti durumu.
    int g_roomType = 0;
    bool g_underwater = false;
    float g_currentCutoff = 20000.0f;
    float g_currentWet = 0.0f;
    float g_currentDecay = 0.0f;
    float g_appliedCutoff = 20000.0f;         // filtreye en son uygulanan kesme frekansi
    uint32_t g_lastEnvironmentTicks = 0;

    float Clamp(float value, float low, float high) {
        return value < low ? low : (value > high ? high : value);
    }

    void ApplyVolumes() {
        ma_engine_set_volume(&g_engine, Clamp(g_CVar.nvs_mastervolume, 0.0f, 1.0f));
        ma_sound_group_set_volume(&g_groups[SOUND_CHANNEL_MUSIC], Clamp(g_CVar.nvs_musicvolume, 0.0f, 1.0f));
    }

    //--------------------------------------------------------------------------
    // Kapatma ve baslatma
    //--------------------------------------------------------------------------
    void Shutdown() {
        if (!g_initialized) return;

        for (auto& active : g_active) ma_sound_uninit(&active->sound);
        g_active.clear();

        for (int i = 0; i < 2; i++) ma_sound_group_uninit(&g_groups[i]);

        if (g_environmentReady) {
            ma_delay_node_uninit(&g_echoNode, nullptr);
            ma_lpf_node_uninit(&g_lowPassNode, nullptr);
            g_environmentReady = false;
        }

        ma_engine_uninit(&g_engine);

        // Kaynak yoneticisi kapandiktan sonra bellek serbest birakilabilir.
        g_encoded.clear();
        g_failedPaths.clear();

        g_initialized = false;
        g_unavailable = true;
    }

    // SFX grubunun cikisini filtre ve yanki uzerinden motorun cikisina baglar.
    void BuildEnvironmentChain() {
        ma_node_graph* graph = &g_engine.nodeGraph;
        const ma_uint32 channels = ma_engine_get_channels(&g_engine);
        const ma_uint32 sampleRate = ma_engine_get_sample_rate(&g_engine);

        // Baslangicta filtre her sey gecirir (20 kHz), yanki kapalidir.
        ma_lpf_node_config lowPassConfig = ma_lpf_node_config_init(channels, sampleRate, 20000.0, 2);
        if (ma_lpf_node_init(graph, &lowPassConfig, nullptr, &g_lowPassNode) != MA_SUCCESS) {
            Console::Log("Oda efekti kurulamadi (filtre); oda efektleri devre disi.");
            return;
        }

        const ma_uint32 delayFrames = static_cast<ma_uint32>(static_cast<float>(sampleRate) * kEchoDelaySeconds);
        ma_delay_node_config echoConfig = ma_delay_node_config_init(channels, sampleRate, delayFrames, 0.0f);
        if (ma_delay_node_init(graph, &echoConfig, nullptr, &g_echoNode) != MA_SUCCESS) {
            Console::Log("Oda efekti kurulamadi (yanki); oda efektleri devre disi.");
            ma_lpf_node_uninit(&g_lowPassNode, nullptr);
            return;
        }

        ma_delay_node_set_dry(&g_echoNode, 1.0f);   // orijinal ses her zaman tam duyulur
        ma_delay_node_set_wet(&g_echoNode, 0.0f);

        ma_node_attach_output_bus(&g_groups[SOUND_CHANNEL_SFX], 0, &g_lowPassNode, 0);
        ma_node_attach_output_bus(&g_lowPassNode, 0, &g_echoNode, 0);
        ma_node_attach_output_bus(&g_echoNode, 0, ma_engine_get_endpoint(&g_engine), 0);

        g_environmentReady = true;
    }

    bool EnsureInit() {
        if (g_initialized) return true;
        if (g_unavailable) return false;

        ma_engine_config config = ma_engine_config_init();
        if (ma_engine_init(&config, &g_engine) != MA_SUCCESS) {
            g_unavailable = true;
            Console::Log("Ses motoru baslatilamadi (ses cihazi bulunamadi mi?). Ses devre disi.");
            return false;
        }

        for (int i = 0; i < 2; i++) ma_sound_group_init(&g_engine, 0, nullptr, &g_groups[i]);
        ma_engine_listener_set_world_up(&g_engine, 0, 0.0f, 1.0f, 0.0f);   // motor Y-up
        BuildEnvironmentChain();
        ApplyVolumes();

        g_initialized = true;

        // Surec kapanirken, statik nesneler yok edilmeden once cihaz duzgunce kapatilir.
        std::atexit(&Shutdown);
        return true;
    }

    //--------------------------------------------------------------------------
    // Dosya yukleme
    //--------------------------------------------------------------------------
    bool EnsureLoaded(const std::string& path) {
        if (g_encoded.count(path) != 0) return true;
        if (g_failedPaths.count(path) != 0) return false;

        std::vector<uint8_t> data = vfs.ReadFile(path);
        if (data.empty()) {
            g_failedPaths.insert(path);
            Console::Log("Ses dosyasi bulunamadi: " + path);
            return false;
        }

        auto inserted = g_encoded.emplace(path, std::move(data));
        const std::vector<uint8_t>& stored = inserted.first->second;

        if (ma_resource_manager_register_encoded_data(ma_engine_get_resource_manager(&g_engine),
            path.c_str(), stored.data(), stored.size()) != MA_SUCCESS) {
            g_encoded.erase(inserted.first);
            g_failedPaths.insert(path);
            return false;
        }
        return true;
    }

    //--------------------------------------------------------------------------
    // Ses yonetimi
    //--------------------------------------------------------------------------
    // Bitmis sesler listeden kaldirilir (dongulu sesler bitmez).
    void ReclaimFinished() {
        for (size_t i = 0; i < g_active.size();) {
            if (ma_sound_at_end(&g_active[i]->sound)) {
                ma_sound_uninit(&g_active[i]->sound);
                g_active.erase(g_active.begin() + static_cast<std::ptrdiff_t>(i));
            }
            else {
                ++i;
            }
        }
    }

    ActiveSound* FindSound(SoundHandle handle) {
        if (handle == 0) return nullptr;

        for (auto& active : g_active) {
            if (active->handle == handle) return active.get();
        }
        return nullptr;
    }

    void RemoveSound(SoundHandle handle) {
        for (size_t i = 0; i < g_active.size(); i++) {
            if (g_active[i]->handle == handle) {
                ma_sound_uninit(&g_active[i]->sound);
                g_active.erase(g_active.begin() + static_cast<std::ptrdiff_t>(i));
                return;
            }
        }
    }

    ActiveSound* CreateSound(const std::string& path, const SoundParams& params) {
        ReclaimFinished();
        if (g_active.size() >= static_cast<size_t>(kMaxActiveSounds)) return nullptr;
        if (!EnsureInit() || !EnsureLoaded(path)) return nullptr;

        const int channelIndex = (params.channel == SOUND_CHANNEL_MUSIC) ? SOUND_CHANNEL_MUSIC : SOUND_CHANNEL_SFX;

        // Kisa efektler tamamen cozulup bellekte tutulur, muzik akis olarak cozulur.
        ma_uint32 flags = (channelIndex == SOUND_CHANNEL_MUSIC) ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
        if (!params.spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

        auto active = std::make_unique<ActiveSound>();
        if (ma_sound_init_from_file(&g_engine, path.c_str(), flags, &g_groups[channelIndex], nullptr, &active->sound) != MA_SUCCESS) {
            if (g_failedPaths.insert(path).second) {
                Console::Log("Ses calinamadi (desteklenmeyen bicim olabilir): " + path);
            }
            return nullptr;
        }

        ma_sound_set_volume(&active->sound, Clamp(params.volume, 0.0f, kMaxVolume));
        ma_sound_set_pitch(&active->sound, Clamp(params.pitch, kMinPitch, kMaxPitch));
        ma_sound_set_looping(&active->sound, params.loop ? MA_TRUE : MA_FALSE);

        if (params.spatial) {
            ma_sound_set_attenuation_model(&active->sound,
                params.linearFalloff ? ma_attenuation_model_linear : ma_attenuation_model_inverse);
            ma_sound_set_min_distance(&active->sound, params.minDistance);
            ma_sound_set_max_distance(&active->sound, params.maxDistance);
            ma_sound_set_rolloff(&active->sound, 1.0f);
        }

        active->handle = g_nextHandle++;
        g_active.push_back(std::move(active));
        return g_active.back().get();
    }

    //--------------------------------------------------------------------------
    // Oda efekti guncellemesi (her karede)
    //--------------------------------------------------------------------------
    void UpdateEnvironment() {
        if (!g_environmentReady) return;

        const uint32_t now = SDL_GetTicks();
        float deltaTime = (g_lastEnvironmentTicks == 0) ? 0.016f : static_cast<float>(now - g_lastEnvironmentTicks) * 0.001f;
        g_lastEnvironmentTicks = now;
        deltaTime = Clamp(deltaTime, 0.0f, 0.1f);

        // Su altindaysa her zaman su oda tipi; degilse haritanin env_sound bolgesi.
        int room = g_underwater ? kUnderwaterRoomType : g_roomType;
        if (room < 0 || room >= kRoomTypeCount) room = 0;
        const RoomPreset& target = kRoomPresets[room];

        // Her deger hedefe yumusakca yaklasir.
        const float blend = Clamp(deltaTime * kEnvironmentBlendPerSecond, 0.0f, 1.0f);
        g_currentCutoff += (target.cutoffHz - g_currentCutoff) * blend;
        g_currentWet += (target.wet - g_currentWet) * blend;
        g_currentDecay += (target.decay - g_currentDecay) * blend;

        ma_delay_node_set_wet(&g_echoNode, g_currentWet);
        ma_delay_node_set_decay(&g_echoNode, g_currentDecay);

        // Filtreyi her karede degil, kesme frekansi belirgin degisince yeniden kur.
        if (std::fabs(g_currentCutoff - g_appliedCutoff) > g_appliedCutoff * kCutoffUpdateThreshold) {
            ma_lpf_config config = ma_lpf_config_init(ma_format_f32, ma_engine_get_channels(&g_engine),
                ma_engine_get_sample_rate(&g_engine), g_currentCutoff, 2);
            ma_lpf_node_reinit(&config, &g_lowPassNode);
            g_appliedCutoff = g_currentCutoff;
        }
    }

}  // namespace

void SoundSystem_SetUnderwater(bool underwater) {
    g_underwater = underwater;
}

class EngineSoundImpl : public IEngineSound {
public:
    bool Precache(const char* vfsPath) override {
        if (vfsPath == nullptr || !EnsureInit()) return false;
        return EnsureLoaded(vfsPath);
    }

    SoundHandle Play2D(const char* vfsPath, float volume, int channel, bool loop) override {
        return Play2DEx(vfsPath, volume, channel, loop, 1.0f);
    }

    SoundHandle Play3D(const char* vfsPath, float x, float y, float z, float volume, bool loop) override {
        return Play3DEx(vfsPath, x, y, z, volume, loop, 1.0f, kDefaultMinDistance, kDefaultMaxDistance);
    }

    SoundHandle Play2DEx(const char* vfsPath, float volume, int channel, bool loop, float pitch) override {
        if (vfsPath == nullptr) return 0;

        SoundParams params;
        params.channel = channel;
        params.loop = loop;
        params.volume = volume;
        params.pitch = pitch;

        ActiveSound* active = CreateSound(vfsPath, params);
        if (active == nullptr) return 0;

        ma_sound_start(&active->sound);
        return active->handle;
    }

    SoundHandle Play3DEx(const char* vfsPath, float x, float y, float z, float volume, bool loop,
        float pitch, float minDistance, float maxDistance) override {
        if (vfsPath == nullptr) return 0;

        SoundParams params;
        params.spatial = true;
        params.loop = loop;
        params.volume = volume;
        params.pitch = pitch;
        params.minDistance = minDistance;
        params.maxDistance = (maxDistance > minDistance) ? maxDistance : minDistance + 1.0f;
        // Eski Play3D sesleri dogal (ters) sonumleme kullanir; Ex surumu sinirda sessizlesir.
        params.linearFalloff = (minDistance != kDefaultMinDistance || maxDistance != kDefaultMaxDistance);

        ActiveSound* active = CreateSound(vfsPath, params);
        if (active == nullptr) return 0;

        ma_sound_set_position(&active->sound, x, y, z);
        ma_sound_start(&active->sound);
        return active->handle;
    }

    void SetPosition(SoundHandle handle, float x, float y, float z) override {
        if (ActiveSound* active = FindSound(handle)) ma_sound_set_position(&active->sound, x, y, z);
    }

    void SetVolume(SoundHandle handle, float volume) override {
        if (ActiveSound* active = FindSound(handle)) ma_sound_set_volume(&active->sound, Clamp(volume, 0.0f, kMaxVolume));
    }

    void SetPitch(SoundHandle handle, float pitch) override {
        if (ActiveSound* active = FindSound(handle)) ma_sound_set_pitch(&active->sound, Clamp(pitch, kMinPitch, kMaxPitch));
    }

    bool IsPlaying(SoundHandle handle) override {
        ActiveSound* active = FindSound(handle);
        return active != nullptr && !ma_sound_at_end(&active->sound);
    }

    void Stop(SoundHandle handle) override {
        RemoveSound(handle);
    }

    void StopAll() override {
        for (auto& active : g_active) ma_sound_uninit(&active->sound);
        g_active.clear();
    }

    void SetListener(float x, float y, float z, float forwardX, float forwardY, float forwardZ) override {
        // Hic ses calinmadiysa cihaz acik degildir; yapilacak is yok.
        if (!g_initialized) return;

        ma_engine_listener_set_position(&g_engine, 0, x, y, z);
        ma_engine_listener_set_direction(&g_engine, 0, forwardX, forwardY, forwardZ);
        ApplyVolumes();   // Ayarlar'daki kaydiricilar cvar'lari degistirir, burada uygulanir
        UpdateEnvironment();
        ReclaimFinished();
    }

    void SetEnvironment(int roomType) override {
        g_roomType = roomType;
    }
};

EXPOSE_SINGLE_INTERFACE(EngineSoundImpl, IEngineSound, ENGINE_SOUND_INTERFACE_VERSION);