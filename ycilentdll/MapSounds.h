#pragma once
#include <glm/glm.hpp>
#include "iclientdll.h"
#include "ienginecommon.h"
#include "ienginesound.h"

// Haritadaki ses entity'lerinin tamamini yonetir: ambient_generic, env_sound, speaker,
// target_cdaudio / trigger_cdaudio ve bunlari tetikleyen trigger_once / trigger_multiple /
// trigger_relay / trigger_auto.
//
// NOT: tetikleyici islemleri simdilik yalnizca SES hedefleri icindir. Oyunun genel entity
// girdi/cikti sistemi (kapilar, butonlar...) server.dll'e yazilinca bu kisim oraya tasinmalidir.
namespace MapSounds {
    // Harita yuklenince cagrilir: entity'leri tarar ve baslangic seslerini baslatir.
    void Init(IEngineClient* engine, IEngineSound* sound, IEngineConsole* console);

    // Her karede cagrilir. playerOrigin: oyuncunun fizik merkezi (tetikleyici temasi icin),
    // eye: goz konumu (duyma mesafesi ve oda efekti icin).
    void Update(float deltaTime, const glm::vec3& playerOrigin, const glm::vec3& eye);

    void Shutdown();
}