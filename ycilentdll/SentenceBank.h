#pragma once
#include <string>
#include <glm/glm.hpp>
#include "iclientdll.h"
#include "ienginesound.h"

// Half-Life tarzi "cumleler": sound/sentences.txt icinde adlandirilmis kelime listeleri.
// Bir cumle, kelime ses dosyalarinin (sound/<dizin>/<kelime>.wav) arka arkaya calinmasidir.
// Dosya yoksa cumle sistemi sessizce devre disi kalir.
namespace SentenceBank {
    // Onceki oynatmalari temizler; ayristirilan dosya korunur.
    void Init(IEngineClient* engine, IEngineSound* sound);
    void Shutdown();

    // Cumleyi ya da bir grubun rastgele uyesini oynatir (grup: sonundaki rakamlar atilinca ad
    // eslesen cumleler). Bulunamazsa false.
    bool Play(const std::string& nameOrGroup, const glm::vec3& position, bool everywhere,
        float volume, float maxDistance);

    // Her karede cagrilir: siradaki kelimeyi calar.
    void Update(float deltaTime);
}