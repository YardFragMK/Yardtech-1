#include "WeaponModels.h"
#include "netentity.h"
#include "console/Console.h"
#include "../game/src/GLBModel.h"
#include <windows.h>
#include <GL/gl.h>
#include <cmath>
#include <string>

namespace {

    // Silah mermisi modelleri. Dosyalar VFS'ten okunur (nvs1/models/weapons/ ya da gbpak.ypak icinden).
    // Model sozlesmesi (glTF): +Y yukari, +Z modelin onu.
    struct ModelDef {
        uint16_t classId;      // hangi mermi sinifi icin
        const char* path;      // VFS yolu
        float scale;           // model biriminden dunya birimine olcek (modelin buyuklugunu ayarlar)
        float yawOffset;       // modelin onunu ucus yonune (yerel +X) cevirmek icin ek donus (derece)
        float tumbleSpeed;     // havada kendi etrafinda donus hizi (derece/sn); 0 = donmez
        bool spinAboutY;       // true: testere gibi Y ekseni etrafinda doner (hizi donusle artar)
    };

    const ModelDef kModels[] = {
        //  sinif                  yol                                 olcek  yaw  tumble  spinY
        { ENT_CLASS_ROCKET,    "models/weapons/rocket.glb",    12.0f, 90.0f,   0.0f, false },   // roket: ucus yonune bakar
        { ENT_CLASS_GRENADE,   "models/weapons/grenade.glb",   10.0f,  0.0f, 540.0f, false },   // el bombasi: hizli takla
        { ENT_CLASS_FOG_BOMB,  "models/weapons/fogbomb.glb",   10.0f,  0.0f, 360.0f, false },
        { ENT_CLASS_LAVA_BOMB, "models/weapons/lavabomb.glb",  10.0f,  0.0f, 360.0f, false },
        { ENT_CLASS_ACID_BOMB, "models/weapons/acidbomb.glb",  10.0f,  0.0f, 360.0f, false },
        { ENT_CLASS_SAWBLADE,  "models/weapons/sawblade.glb",  24.0f,  0.0f,   0.0f, true  }    // testere: Y ekseninde doner
    };

    constexpr size_t kModelCount = sizeof(kModels) / sizeof(kModels[0]);

    // Testerenin donus hizi: bos testere dakikada 2 devir, tam donmus 6 devir gibi (derece/sn).
    constexpr float kSawBaseSpin = 720.0f;       // donmemis testerenin gorsel donusu
    constexpr float kSawExtraSpin = 1440.0f;     // tam donusle eklenen hiz

    GLBModel g_models[kModelCount];
    bool g_loadAttempted[kModelCount] = {};   // her model yalnizca bir kez yuklenmeyi dener

    // Modeller GL kaynagi olusturdugu icin yukleme ilk cizimde yapilir.
    void EnsureLoadAttempted(size_t index) {
        if (g_loadAttempted[index]) return;

        g_loadAttempted[index] = true;
        g_models[index].Load(kModels[index].path);
    }

}  // namespace

bool WeaponModels::Draw(uint16_t classId, float timeSeconds, float spin01) {
    for (size_t i = 0; i < kModelCount; i++) {
        const ModelDef& def = kModels[i];
        if (def.classId != classId) continue;

        EnsureLoadAttempted(i);
        if (!g_models[i].IsLoaded()) return false;   // dosya yok: hicbir sey cizilmez

        glPushMatrix();
        if (def.spinAboutY) {
            const float angle = std::fmod(timeSeconds * (kSawBaseSpin + kSawExtraSpin * spin01), 360.0f);
            glRotatef(angle, 0.0f, 1.0f, 0.0f);
        }
        else if (def.tumbleSpeed > 0.0f) {
            glRotatef(std::fmod(timeSeconds * def.tumbleSpeed, 360.0f), 1.0f, 0.0f, 0.3f);   // hafif egik eksen: dogal takla
        }
        glRotatef(def.yawOffset, 0.0f, 1.0f, 0.0f);
        glScalef(def.scale, def.scale, def.scale);
        g_models[i].Render();
        glPopMatrix();
        return true;
    }
    return false;
}

bool WeaponModels::IsAvailable(uint16_t classId) {
    for (size_t i = 0; i < kModelCount; i++) {
        if (kModels[i].classId != classId) continue;

        EnsureLoadAttempted(i);
        return g_models[i].IsLoaded();
    }
    return false;
}

void WeaponModels::LogStatus() {
    for (size_t i = 0; i < kModelCount; i++) {
        EnsureLoadAttempted(i);
        Console::Log(std::string("model ") + kModels[i].path + ": " + (g_models[i].IsLoaded() ? "var" : "YOK"));
    }
}