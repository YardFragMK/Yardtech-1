#include "pch.h"
#include "MapSounds.h"
#include "SentenceBank.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

    //==========================================================================
    // SABITLER
    //==========================================================================

    // --- ambient_generic ---
    constexpr float kRadiusSmall = 500.0f;     // "Kucuk yaricap" bayragi: duyulma mesafesi (birim)
    constexpr float kRadiusMedium = 800.0f;    // "Orta yaricap" (hicbir boyut bayragi yoksa da bu kullanilir)
    constexpr float kRadiusLarge = 1250.0f;    // "Buyuk yaricap" bayragi
    constexpr float kMinRadius = 64.0f;        // "radius" anahtarinin alt siniri
    constexpr float kMaxRadius = 8192.0f;      // ust siniri
    constexpr float kFullVolumeFraction = 0.25f;   // ses, yaricapinin %25'ine kadar tam siddette; sonra dogrusal azalir
    constexpr float kRangeHysteresis = 1.05f;  // dongulu ses yaricapin %5 disina cikinca durdurulur (sinirda ac-kapa titremesi olmasin)

    // Perde ve ses seviyesi anahtarlarinin donusumu
    constexpr float kVolumeKeyMax = 10.0f;     // "health" 0..10 -> ses 0..1
    constexpr float kPitchKeyNormal = 100.0f;  // "pitch" 100 = normal perde
    constexpr float kMinPitch = 0.25f;
    constexpr float kMaxPitch = 4.0f;

    // Fade suresi: anahtar degeri N ise sure 100/N saniye (N=100 -> 1 sn, N=10 -> 10 sn).
    constexpr float kFadeMinSeconds = 0.2f;
    constexpr float kFadeMaxSeconds = 30.0f;

    // LFO (ses ve perdeyi salindiran dusuk frekansli osilator)
    constexpr float kLfoRateDivisor = 100.0f;  // "lforate" 0..1000 -> 0..10 Hz
    constexpr float kLfoPitchSwing = 0.5f;     // "lfomodpitch" 100 = perde +-%50 salinir
    constexpr float kLfoVolumeSwing = 1.0f;    // "lfomodvol" 100 = ses +-%100 salinir
    constexpr float kLfoPercentDivisor = 100.0f;

    // --- speaker ---
    constexpr float kSpeakerRange = 2000.0f;           // anonslarin duyulma mesafesi (birim)
    constexpr float kSpeakerDefaultDelayMin = 15.0f;   // haritada belirtilmediyse iki anons arasi en az sure (sn)
    constexpr float kSpeakerDefaultDelayMax = 135.0f;  // en cok sure (sn)

    // --- env_sound ---
    constexpr float kEnvironmentCheckInterval = 0.1f;  // oda efekti bolgeleri kac saniyede bir denetlenir

    // --- tetikleyiciler ---
    const glm::vec3 kPlayerHalfExtents(16.0f, 36.0f, 16.0f);   // oyuncunun temas kutusu (ayakta, fizik merkezinden)
    constexpr float kDefaultTriggerWait = 0.2f;        // trigger_multiple: tekrar tetiklenmeden once bekleme (sn)
    constexpr int kMaxTriggerChainDepth = 8;           // trigger_relay zincirinin en fazla derinligi (sonsuz dongu korumasi)
    constexpr float kTouchEpsilon = 0.01f;             // kutu cakismasi hassasiyeti (birim)

    // --- muzik ---
    constexpr int kFirstAudioTrack = 2;    // 0 ve 1 "muzigi durdur" demektir (1: CD'deki veri izi)
    constexpr float kMusicVolume = 1.0f;

    // Spawnflags (Half-Life FGD ile ayni)
    constexpr int kAmbientEverywhere = 1;      // her yerden duyulur (konumsuz)
    constexpr int kAmbientSmallRadius = 2;
    constexpr int kAmbientMediumRadius = 4;
    constexpr int kAmbientLargeRadius = 8;
    constexpr int kAmbientStartSilent = 16;    // sessiz baslar; tetikleyici bekler
    constexpr int kAmbientNotLooped = 32;      // dongulu degil
    constexpr int kSpeakerStartSilent = 1;
    constexpr int kTriggerNoClients = 2;       // oyuncu tetikleyemez

    //==========================================================================
    // GENEL DURUM
    //==========================================================================
    IEngineClient* g_engine = nullptr;
    IEngineSound* g_sound = nullptr;
    IEngineConsole* g_console = nullptr;
    IWorldQuery* g_world = nullptr;
    float g_time = 0.0f;               // haritanin yuklendiginden beri gecen sure (sn)

    //==========================================================================
    // YARDIMCILAR
    //==========================================================================
    float ClampF(float value, float low, float high) { return value < low ? low : (value > high ? high : value); }
    float MinF(float a, float b) { return a < b ? a : b; }
    float MaxF(float a, float b) { return a > b ? a : b; }

    std::mt19937& RandomEngine() {
        static std::mt19937 engine{ std::random_device{}() };
        return engine;
    }

    float RandomRange(float low, float high) {
        if (high <= low) return low;
        return std::uniform_real_distribution<float>(low, high)(RandomEngine());
    }

    std::string ToLower(std::string text) {
        for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    // Hedef adlari buyuk/kucuk harfe duyarsiz karsilastirilir; bos adlar hicbir seyle eslesmez.
    bool SameName(const std::string& a, const std::string& b) {
        return !a.empty() && ToLower(a) == ToLower(b);
    }

    // --- Harita anahtarlari ---
    std::string KeyText(int entityIndex, const char* key) {
        const char* value = g_world->GetEntityKeyValue(entityIndex, key);
        return value != nullptr ? std::string(value) : std::string();
    }

    float KeyFloat(int entityIndex, const char* key, float fallback) {
        const char* value = g_world->GetEntityKeyValue(entityIndex, key);
        if (value == nullptr || value[0] == '\0') return fallback;
        return static_cast<float>(std::atof(value));
    }

    int KeyInt(int entityIndex, const char* key, int fallback) {
        return static_cast<int>(KeyFloat(entityIndex, key, static_cast<float>(fallback)));
    }

    // BSP'nin "x y z" konumunu motor uzayina (Y yukari) cevirir.
    glm::vec3 ParseBspOrigin(const std::string& text) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        std::istringstream stream(text);
        stream >> x >> y >> z;
        return glm::vec3(x, z, -y);
    }

    bool ReadOrigin(int entityIndex, glm::vec3& outOrigin) {
        const std::string text = KeyText(entityIndex, "origin");
        if (text.empty()) return false;

        outOrigin = ParseBspOrigin(text);
        return true;
    }

    // "ambience/wind.wav" -> "sound/ambience/wind.wav"
    std::string SoundPathFor(const std::string& message) {
        std::string path = message;
        for (char& c : path) if (c == '\\') c = '/';

        if (path.compare(0, 6, "sound/") != 0) path = "sound/" + path;
        return path;
    }

    float FadeSecondsFromKey(float keyValue) {
        if (keyValue <= 0.0f) return 0.0f;   // 0 = fade yok
        return ClampF(100.0f / keyValue, kFadeMinSeconds, kFadeMaxSeconds);
    }

    //==========================================================================
    // VERI TIPLERI
    //==========================================================================
    struct AmbientSound {
        // --- Haritadan ---
        std::string targetName;
        std::string soundPath;         // dosya sesi icin "sound/..." yolu
        std::string sentenceName;      // "!" ile baslayan mesajlarda cumle ya da grup adi
        glm::vec3 position{ 0.0f };
        bool playEverywhere = false;
        float maxDistance = kRadiusMedium;    // duyulma yaricapi
        float fullVolumeDistance = kRadiusMedium * kFullVolumeFraction;
        float baseVolume = 1.0f;       // 0..1
        float basePitch = 1.0f;
        bool looped = true;
        bool startSilent = false;
        float fadeInSeconds = 0.0f;
        float fadeOutSeconds = 0.0f;
        int lfoType = 0;               // 0 yok, 1 kare, 2 ucgen, 3 rastgele
        float lfoRateHz = 0.0f;
        float lfoPitchDepth = 0.0f;    // perdenin +- ne kadar salinacagi (oran)
        float lfoVolumeDepth = 0.0f;

        // --- Calisma durumu ---
        bool wanted = false;           // dongulu ses calmali mi (tetikleyiciler bunu degistirir)
        bool playOncePending = false;  // dongulu olmayan, sessiz baslamayan ses: duyulacak mesafeye gelince bir kez calar
        bool loadFailed = false;       // dosya acilamadi; tekrar denenmez
        SoundHandle handle = 0;
        float gain = 0.0f;             // fade ile 0..1 arasi degisen carpan
        float lfoPhase = 0.0f;
        float lfoRandomValue = 0.0f;
        float lfoRandomTimer = 0.0f;
    };

    struct Speaker {
        std::string targetName;
        std::string message;           // dosya yolu ya da "!cumle"
        glm::vec3 position{ 0.0f };
        float volume = 1.0f;
        float delayMin = kSpeakerDefaultDelayMin;
        float delayMax = kSpeakerDefaultDelayMax;
        float timer = 0.0f;            // bir sonraki anonsa kalan sure (sn)
        bool startSilent = false;      // true: yalnizca tetiklenince konusur
    };

    struct EnvironmentZone {
        glm::vec3 position{ 0.0f };
        float radius = 0.0f;
        int roomType = 0;              // Half-Life oda tipi 0..28
    };

    // target_cdaudio: oyuncu yaricap icine girince (ya da tetiklenince) bir muzik parcasi calar.
    struct MusicZone {
        std::string targetName;
        glm::vec3 position{ 0.0f };
        float radius = 0.0f;           // 0 = yalnizca tetiklenince
        int track = 0;
        bool playerInside = false;
    };

    enum class TriggerKind { Once, Multiple, CdAudio };

    // Brush hacimli tetikleyici: oyuncu hacme girince hedefini tetikler.
    struct BrushTrigger {
        TriggerKind kind = TriggerKind::Once;
        std::string target;
        glm::vec3 mins{ 0.0f };
        glm::vec3 maxs{ 0.0f };
        float delay = 0.0f;            // hedef tetiklenmeden once bekleme (sn)
        float wait = kDefaultTriggerWait;   // Multiple: tekrar tetiklenme araligi (sn)
        float cooldown = 0.0f;         // bir sonraki tetiklemeye kalan sure
        int cdTrack = -1;              // CdAudio: calinacak parca
        bool done = false;             // Once: bir kez tetiklendi
    };

    struct Relay {
        std::string targetName;
        std::string target;
        float delay = 0.0f;
    };

    struct ScheduledFire {
        float fireTime = 0.0f;         // g_time bu degere ulasinca
        std::string target;
        int depth = 0;
    };

    std::vector<AmbientSound> g_ambients;
    std::vector<Speaker> g_speakers;
    std::vector<EnvironmentZone> g_environmentZones;
    std::vector<MusicZone> g_musicZones;
    std::vector<BrushTrigger> g_triggers;
    std::vector<Relay> g_relays;
    std::vector<ScheduledFire> g_scheduled;

    SoundHandle g_music = 0;
    float g_environmentTimer = 0.0f;
    int g_unsupportedCount = 0;        // desteklenmeyen mesajlar ("*" ile baslayanlar)

    //==========================================================================
    // AMBIENT_GENERIC
    //==========================================================================
    SoundHandle StartAmbientVoice(const AmbientSound& ambient, bool loop) {
        if (ambient.playEverywhere) {
            return g_sound->Play2DEx(ambient.soundPath.c_str(), ambient.baseVolume, SOUND_CHANNEL_SFX, loop, ambient.basePitch);
        }
        return g_sound->Play3DEx(ambient.soundPath.c_str(), ambient.position.x, ambient.position.y, ambient.position.z,
            ambient.baseVolume, loop, ambient.basePitch, ambient.fullVolumeDistance, ambient.maxDistance);
    }

    // Dongulu olmayan sesi (ya da cumleyi) bir kez calar.
    void PlayAmbientOnce(AmbientSound& ambient) {
        if (!ambient.sentenceName.empty()) {
            SentenceBank::Play(ambient.sentenceName, ambient.position, ambient.playEverywhere,
                ambient.baseVolume, ambient.maxDistance);
            return;
        }
        if (ambient.loadFailed) return;

        if (StartAmbientVoice(ambient, false) == 0) ambient.loadFailed = true;
    }

    // Tetiklenince: dongulu ses acilip kapanir (toggle), digerleri bir kez calar.
    void FireAmbient(AmbientSound& ambient) {
        const bool toggles = ambient.looped && ambient.sentenceName.empty();
        if (toggles) ambient.wanted = !ambient.wanted;
        else PlayAmbientOnce(ambient);
    }

    // LFO dalgasi: -1..1 arasi bir deger. Tur 1 kare, 2 ucgen, 3 her dongude yeni rastgele deger.
    float LfoWave(AmbientSound& ambient, float deltaTime) {
        ambient.lfoPhase += deltaTime * ambient.lfoRateHz;
        const float cycle = ambient.lfoPhase - std::floor(ambient.lfoPhase);   // mevcut dongudeki konum 0..1

        switch (ambient.lfoType) {
        case 1:  return cycle < 0.5f ? 1.0f : -1.0f;               // kare
        case 2:  return 4.0f * std::fabs(cycle - 0.5f) - 1.0f;     // ucgen
        case 3:
            ambient.lfoRandomTimer -= deltaTime;
            if (ambient.lfoRandomTimer <= 0.0f) {
                ambient.lfoRandomValue = RandomRange(-1.0f, 1.0f);
                ambient.lfoRandomTimer = 1.0f / ambient.lfoRateHz;   // her dongude bir yeni deger
            }
            return ambient.lfoRandomValue;
        default: return 0.0f;
        }
    }

    void UpdateAmbient(AmbientSound& ambient, float deltaTime, const glm::vec3& eye) {
        // Fade: carpan hedefe (acik = 1, kapali = 0) fade suresine gore yaklasir.
        const float target = ambient.wanted ? 1.0f : 0.0f;
        if (target > ambient.gain) {
            ambient.gain = (ambient.fadeInSeconds > 0.0f) ? MinF(target, ambient.gain + deltaTime / ambient.fadeInSeconds) : target;
        }
        else if (target < ambient.gain) {
            ambient.gain = (ambient.fadeOutSeconds > 0.0f) ? MaxF(target, ambient.gain - deltaTime / ambient.fadeOutSeconds) : target;
        }

        const float distance = ambient.playEverywhere ? 0.0f : glm::length(eye - ambient.position);
        const bool inRange = distance < ambient.maxDistance * kRangeHysteresis;

        // Dongulu olmayan ses, duyulacak mesafeye gelince bir kez calar.
        if (ambient.playOncePending && inRange) {
            ambient.playOncePending = false;
            PlayAmbientOnce(ambient);
        }

        // Dongulu sesler (cumleler haric) yalnizca duyulabilecek mesafedeyken calinir: harita
        // boyunca yuzlerce ambient olsa bile ses sayisi sinirda kalir.
        if (!ambient.looped || !ambient.sentenceName.empty()) return;

        const bool shouldPlay = (ambient.wanted || ambient.gain > 0.001f) && inRange && !ambient.loadFailed;

        if (shouldPlay && ambient.handle == 0) {
            ambient.handle = StartAmbientVoice(ambient, true);
            if (ambient.handle == 0) ambient.loadFailed = true;
        }
        else if (!shouldPlay && ambient.handle != 0) {
            g_sound->Stop(ambient.handle);
            ambient.handle = 0;
        }

        if (ambient.handle == 0) return;

        float volume = ambient.baseVolume * ambient.gain;
        float pitch = ambient.basePitch;

        if (ambient.lfoType != 0 && ambient.lfoRateHz > 0.0f) {
            const float wave = LfoWave(ambient, deltaTime);
            volume *= MaxF(0.0f, 1.0f + wave * ambient.lfoVolumeDepth);
            pitch *= MaxF(kMinPitch, 1.0f + wave * ambient.lfoPitchDepth);
        }

        g_sound->SetVolume(ambient.handle, volume);
        g_sound->SetPitch(ambient.handle, pitch);
    }

    void ParseAmbient(int entityIndex) {
        const std::string message = KeyText(entityIndex, "message");
        if (message.empty()) return;

        AmbientSound ambient;
        ambient.targetName = KeyText(entityIndex, "targetname");
        const int flags = KeyInt(entityIndex, "spawnflags", 0);

        if (message[0] == '!') {
            ambient.sentenceName = message.substr(1);      // "!CUMLE": sentences.txt'ten
        }
        else if (message[0] == '*') {
            ++g_unsupportedCount;                          // "*grup": rastgele ses grubu, desteklenmiyor
            return;
        }
        else {
            ambient.soundPath = SoundPathFor(message);
        }

        ambient.playEverywhere = (flags & kAmbientEverywhere) != 0;
        if (!ambient.playEverywhere && !ReadOrigin(entityIndex, ambient.position)) return;

        // Duyulma yaricapi: "radius" anahtari varsa o, yoksa boyut bayragi, o da yoksa orta.
        float radius = kRadiusMedium;
        if (flags & kAmbientSmallRadius) radius = kRadiusSmall;
        else if (flags & kAmbientLargeRadius) radius = kRadiusLarge;

        const float customRadius = KeyFloat(entityIndex, "radius", 0.0f);
        if (customRadius > 0.0f) radius = ClampF(customRadius, kMinRadius, kMaxRadius);

        ambient.maxDistance = radius;
        ambient.fullVolumeDistance = radius * kFullVolumeFraction;

        // "health" 0..10 ses seviyesidir (varsayilan 10 = tam); "pitch" 100 = normal (0 ise normal sayilir).
        ambient.baseVolume = ClampF(KeyFloat(entityIndex, "health", kVolumeKeyMax) / kVolumeKeyMax, 0.0f, 1.0f);
        float pitchKey = KeyFloat(entityIndex, "pitch", kPitchKeyNormal);
        if (pitchKey <= 0.0f) pitchKey = kPitchKeyNormal;
        ambient.basePitch = ClampF(pitchKey / kPitchKeyNormal, kMinPitch, kMaxPitch);

        ambient.looped = (flags & kAmbientNotLooped) == 0;
        ambient.startSilent = (flags & kAmbientStartSilent) != 0;
        ambient.fadeInSeconds = FadeSecondsFromKey(KeyFloat(entityIndex, "fadein", 0.0f));
        ambient.fadeOutSeconds = FadeSecondsFromKey(KeyFloat(entityIndex, "fadeout", 0.0f));

        ambient.lfoType = KeyInt(entityIndex, "lfotype", 0);
        ambient.lfoRateHz = ClampF(KeyFloat(entityIndex, "lforate", 0.0f) / kLfoRateDivisor, 0.0f, 10.0f);
        ambient.lfoPitchDepth = ClampF(KeyFloat(entityIndex, "lfomodpitch", 0.0f) / kLfoPercentDivisor, 0.0f, 1.0f) * kLfoPitchSwing;
        ambient.lfoVolumeDepth = ClampF(KeyFloat(entityIndex, "lfomodvol", 0.0f) / kLfoPercentDivisor, 0.0f, 1.0f) * kLfoVolumeSwing;

        ambient.wanted = ambient.looped && !ambient.startSilent;
        ambient.playOncePending = !ambient.looped && !ambient.startSilent;
        g_ambients.push_back(ambient);
    }

    //==========================================================================
    // SPEAKER
    //==========================================================================
    void PlaySpeaker(Speaker& speaker) {
        if (speaker.message.empty()) return;

        if (speaker.message[0] == '!') {
            SentenceBank::Play(speaker.message.substr(1), speaker.position, false, speaker.volume, kSpeakerRange);
            return;
        }
        g_sound->Play3DEx(SoundPathFor(speaker.message).c_str(), speaker.position.x, speaker.position.y, speaker.position.z,
            speaker.volume, false, 1.0f, kSpeakerRange * kFullVolumeFraction, kSpeakerRange);
    }

    void ScheduleNextAnnouncement(Speaker& speaker) {
        speaker.timer = RandomRange(speaker.delayMin, speaker.delayMax);
    }

    void UpdateSpeaker(Speaker& speaker, float deltaTime, const glm::vec3& eye) {
        if (speaker.startSilent) return;

        speaker.timer -= deltaTime;
        if (speaker.timer > 0.0f) return;

        // Cok uzaktaysa duyulmaz: ses kaynagi bos yere harcanmasin.
        if (glm::length(eye - speaker.position) < kSpeakerRange) PlaySpeaker(speaker);
        ScheduleNextAnnouncement(speaker);
    }

    void ParseSpeaker(int entityIndex) {
        Speaker speaker;
        speaker.message = KeyText(entityIndex, "message");
        if (speaker.message.empty() || !ReadOrigin(entityIndex, speaker.position)) return;

        speaker.targetName = KeyText(entityIndex, "targetname");
        speaker.volume = ClampF(KeyFloat(entityIndex, "health", kVolumeKeyMax) / kVolumeKeyMax, 0.0f, 1.0f);
        speaker.delayMin = KeyFloat(entityIndex, "delaymin", kSpeakerDefaultDelayMin);
        speaker.delayMax = MaxF(speaker.delayMin, KeyFloat(entityIndex, "delaymax", kSpeakerDefaultDelayMax));
        speaker.startSilent = (KeyInt(entityIndex, "spawnflags", 0) & kSpeakerStartSilent) != 0;

        ScheduleNextAnnouncement(speaker);
        g_speakers.push_back(speaker);
    }

    //==========================================================================
    // ENV_SOUND (oda efekti)
    //==========================================================================
    void ParseEnvironmentZone(int entityIndex) {
        EnvironmentZone zone;
        if (!ReadOrigin(entityIndex, zone.position)) return;

        zone.radius = KeyFloat(entityIndex, "radius", 0.0f);
        zone.roomType = KeyInt(entityIndex, "roomtype", 0);
        if (zone.radius > 0.0f) g_environmentZones.push_back(zone);
    }

    // Oyuncu hangi bolgenin icinde? Birden fazla varsa en yakin ve goz hattinda olan secilir.
    void UpdateEnvironment(float deltaTime, const glm::vec3& eye) {
        g_environmentTimer -= deltaTime;
        if (g_environmentTimer > 0.0f) return;
        g_environmentTimer = kEnvironmentCheckInterval;

        int roomType = 0;
        float bestDistance = 1.0e9f;

        for (const EnvironmentZone& zone : g_environmentZones) {
            const float distance = glm::length(eye - zone.position);
            if (distance >= zone.radius || distance >= bestDistance) continue;

            // Duvar arkasindaki bolge etkili olmaz (Half-Life'taki gibi gorus hatti sartı).
            const TraceResult trace = g_world->TraceLine(eye, zone.position, 0);
            if (trace.fraction < 1.0f) continue;

            bestDistance = distance;
            roomType = zone.roomType;
        }

        g_sound->SetEnvironment(roomType);
    }

    //==========================================================================
    // MUZIK (target_cdaudio / trigger_cdaudio)
    //==========================================================================
    // Parca numarasi -> "media/trackNN.<uzanti>". Bulunan ilk uzanti calinir.
    void PlayTrack(int track) {
        if (g_music != 0) {
            g_sound->Stop(g_music);
            g_music = 0;
        }
        if (track < kFirstAudioTrack) return;   // 0 ve 1: muzigi durdur

        static const char* const kExtensions[] = { "mp3", "ogg", "wav", "flac" };
        char path[96];

        for (const char* extension : kExtensions) {
            std::snprintf(path, sizeof(path), "media/track%02d.%s", track, extension);
            if (g_engine->GetFileSize(path) < 0) continue;

            g_music = g_sound->Play2DEx(path, kMusicVolume, SOUND_CHANNEL_MUSIC, false, 1.0f);
            return;
        }

        if (g_console != nullptr) {
            std::snprintf(path, sizeof(path), "client.dll: muzik izi bulunamadi: media/track%02d.*", track);
            g_console->Print(path);
        }
    }

    void ParseMusicZone(int entityIndex) {
        MusicZone zone;
        zone.targetName = KeyText(entityIndex, "targetname");
        zone.track = KeyInt(entityIndex, "health", 0);
        zone.radius = KeyFloat(entityIndex, "radius", 0.0f);
        ReadOrigin(entityIndex, zone.position);
        g_musicZones.push_back(zone);
    }

    void UpdateMusicZones(const glm::vec3& playerOrigin) {
        for (MusicZone& zone : g_musicZones) {
            if (zone.radius <= 0.0f) continue;   // yalnizca tetiklenince calan bolge

            const bool inside = glm::length(playerOrigin - zone.position) < zone.radius;
            if (inside && !zone.playerInside) PlayTrack(zone.track);   // her girisinde bir kez
            zone.playerInside = inside;
        }
    }

    //==========================================================================
    // TETIKLEYICILER
    //==========================================================================
    void Schedule(const std::string& target, float delay, int depth) {
        if (target.empty()) return;

        ScheduledFire fire;
        fire.fireTime = g_time + MaxF(0.0f, delay);
        fire.target = target;
        fire.depth = depth;
        g_scheduled.push_back(fire);
    }

    // Adi eslesen tum ses entity'lerini tetikler.
    void Fire(const std::string& target, int depth) {
        if (target.empty() || depth > kMaxTriggerChainDepth) return;

        for (AmbientSound& ambient : g_ambients) {
            if (SameName(ambient.targetName, target)) FireAmbient(ambient);
        }
        for (Speaker& speaker : g_speakers) {
            if (SameName(speaker.targetName, target)) PlaySpeaker(speaker);
        }
        for (MusicZone& zone : g_musicZones) {
            if (SameName(zone.targetName, target)) PlayTrack(zone.track);
        }
        for (const Relay& relay : g_relays) {
            if (SameName(relay.targetName, target)) Schedule(relay.target, relay.delay, depth + 1);
        }
    }

    void ParseBrushTrigger(int entityIndex, const std::string& className) {
        if (KeyInt(entityIndex, "spawnflags", 0) & kTriggerNoClients) return;   // oyuncu tetikleyemez

        // Brush entity'lerin modeli "*N" bicimindedir.
        const std::string model = KeyText(entityIndex, "model");
        if (model.empty() || model[0] != '*') return;

        BrushTrigger trigger;
        if (!g_world->GetBrushModelBounds(std::atoi(model.c_str() + 1), &trigger.mins, &trigger.maxs)) return;

        trigger.target = KeyText(entityIndex, "target");
        trigger.delay = KeyFloat(entityIndex, "delay", 0.0f);

        if (className == "trigger_cdaudio") {
            trigger.kind = TriggerKind::CdAudio;
            trigger.cdTrack = KeyInt(entityIndex, "health", 0);
        }
        else if (className == "trigger_once") {
            trigger.kind = TriggerKind::Once;
        }
        else {
            trigger.kind = TriggerKind::Multiple;
            trigger.wait = KeyFloat(entityIndex, "wait", kDefaultTriggerWait);
            if (trigger.wait < 0.0f) trigger.kind = TriggerKind::Once;   // wait = -1: yalnizca bir kez
        }
        g_triggers.push_back(trigger);
    }

    bool BoxesOverlap(const glm::vec3& minsA, const glm::vec3& maxsA, const glm::vec3& minsB, const glm::vec3& maxsB) {
        return minsA.x <= maxsB.x + kTouchEpsilon && maxsA.x >= minsB.x - kTouchEpsilon
            && minsA.y <= maxsB.y + kTouchEpsilon && maxsA.y >= minsB.y - kTouchEpsilon
            && minsA.z <= maxsB.z + kTouchEpsilon && maxsA.z >= minsB.z - kTouchEpsilon;
    }

    void UpdateTriggers(float deltaTime, const glm::vec3& playerOrigin) {
        const glm::vec3 playerMins = playerOrigin - kPlayerHalfExtents;
        const glm::vec3 playerMaxs = playerOrigin + kPlayerHalfExtents;

        for (BrushTrigger& trigger : g_triggers) {
            if (trigger.done) continue;

            trigger.cooldown -= deltaTime;
            if (trigger.cooldown > 0.0f) continue;
            if (!BoxesOverlap(playerMins, playerMaxs, trigger.mins, trigger.maxs)) continue;

            // Tetiklendi: "Once" turleri kalici olarak kapanir, "Multiple" bekleme suresine girer.
            const bool oneShot = (trigger.kind != TriggerKind::Multiple);
            trigger.done = oneShot;
            trigger.cooldown = oneShot ? 0.0f : trigger.wait;

            if (trigger.kind == TriggerKind::CdAudio) PlayTrack(trigger.cdTrack);
            Schedule(trigger.target, trigger.delay, 0);
        }
    }

    // Zamani gelen gecikmeli tetiklemeleri uygular.
    void UpdateScheduled() {
        for (size_t i = 0; i < g_scheduled.size();) {
            if (g_time < g_scheduled[i].fireTime) { ++i; continue; }

            // Once listeden cikar: Fire yeni gecikmeli tetiklemeler ekleyebilir.
            const ScheduledFire fire = g_scheduled[i];
            g_scheduled.erase(g_scheduled.begin() + static_cast<std::ptrdiff_t>(i));
            Fire(fire.target, fire.depth);
        }
    }

    //==========================================================================
    // HARITA TARAMASI
    //==========================================================================
    void ScanEntities() {
        const int count = g_world->GetEntityCount();

        for (int i = 0; i < count; ++i) {
            const std::string className = KeyText(i, "classname");

            if (className == "ambient_generic") ParseAmbient(i);
            else if (className == "speaker") ParseSpeaker(i);
            else if (className == "env_sound") ParseEnvironmentZone(i);
            else if (className == "target_cdaudio") ParseMusicZone(i);
            else if (className == "trigger_cdaudio" || className == "trigger_once" || className == "trigger_multiple") {
                ParseBrushTrigger(i, className);
            }
            else if (className == "trigger_relay") {
                Relay relay;
                relay.targetName = KeyText(i, "targetname");
                relay.target = KeyText(i, "target");
                relay.delay = KeyFloat(i, "delay", 0.0f);
                g_relays.push_back(relay);
            }
            else if (className == "trigger_auto") {
                // Harita yuklenince hedefini kendiliginden tetikler (ornegin ortam sesini baslatir).
                Schedule(KeyText(i, "target"), KeyFloat(i, "delay", 0.0f), 0);
            }
        }
    }

    void ClearAll() {
        g_ambients.clear();
        g_speakers.clear();
        g_environmentZones.clear();
        g_musicZones.clear();
        g_triggers.clear();
        g_relays.clear();
        g_scheduled.clear();
        g_music = 0;
        g_environmentTimer = 0.0f;
        g_unsupportedCount = 0;
        g_time = 0.0f;
    }

}  // namespace

void MapSounds::Init(IEngineClient* engine, IEngineSound* sound, IEngineConsole* console) {
    ClearAll();

    g_engine = engine;
    g_sound = sound;
    g_console = console;
    g_world = (engine != nullptr) ? engine->GetWorld() : nullptr;

    SentenceBank::Init(engine, sound);
    if (g_world == nullptr || g_sound == nullptr) return;

    ScanEntities();

    if (g_console != nullptr && (!g_ambients.empty() || !g_speakers.empty() || !g_environmentZones.empty()
        || !g_musicZones.empty() || !g_triggers.empty())) {
        const std::string summary = "client.dll: harita sesleri: ambient " + std::to_string(g_ambients.size())
            + ", speaker " + std::to_string(g_speakers.size())
            + ", env_sound " + std::to_string(g_environmentZones.size())
            + ", cdaudio " + std::to_string(g_musicZones.size())
            + ", tetikleyici " + std::to_string(g_triggers.size())
            + ", desteklenmeyen " + std::to_string(g_unsupportedCount);
        g_console->Print(summary.c_str());
    }
}

void MapSounds::Update(float deltaTime, const glm::vec3& playerOrigin, const glm::vec3& eye) {
    if (g_world == nullptr || g_sound == nullptr) return;

    g_time += deltaTime;

    UpdateTriggers(deltaTime, playerOrigin);
    UpdateScheduled();
    UpdateMusicZones(playerOrigin);
    UpdateEnvironment(deltaTime, eye);

    for (AmbientSound& ambient : g_ambients) UpdateAmbient(ambient, deltaTime, eye);
    for (Speaker& speaker : g_speakers) UpdateSpeaker(speaker, deltaTime, eye);

    SentenceBank::Update(deltaTime);
}

void MapSounds::Shutdown() {
    ClearAll();
    SentenceBank::Shutdown();
    g_world = nullptr;
    g_engine = nullptr;
    g_sound = nullptr;
    g_console = nullptr;
}