#pragma once
#include <cstdint>

// Silah mermilerinin GLB modelleri. Dosyalar nvs1/models/weapons/ altinda aranir; yoksa
// Draw false doner ve cagiran taraf yedek primitifi cizer.
namespace WeaponModels {
    // Yerel koordinatin sifir noktasinda cizer. Cagiran, once cevirmeyi (ve roket icin
    // ucus yonunu) uygulamis olmalidir. spin01: testere donusu (0..1).
    bool Draw(uint16_t classId, float timeSeconds, float spin01);

    // Prints which weapon model files exist ("fx_status").
    void LogStatus();

    bool IsAvailable(uint16_t classId);
}