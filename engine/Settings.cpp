#include "Settings.h"
#include "yg_ygui.h"
#include "ConVarRegistry.h"
#include "console/CVar.h"
#include "renderer/Renderer.h"
#include "HUD.h"

extern Renderer renderer;

bool Settings::s_open = false;
Settings::Category Settings::s_activeCategory = Settings::Category::Graphics;

namespace {
    constexpr float WINDOW_WIDTH = 560.0f;
    constexpr float TAB_WIDTH = 120.0f;
}

void Settings::Open() {
    s_open = true;
}

void Settings::Close() {
    s_open = false;
}

bool Settings::IsOpen() {
    return s_open;
}

void Settings::DrawGraphicsTab() {
    bool retroMode = renderer.IsRetroMode();
    if (YGUI::DrawCheckBox("RETRO MODE", &retroMode)) {
        renderer.SetRetroMode(retroMode);
    }
    if (float* crt = ConVarRegistry::FindFloat("r_crt")) {
        YGUI::DrawSliderFloat("CRT EFFECT", crt, 0.0f, 1.0f);
    }
    if (float* dream = ConVarRegistry::FindFloat("r_dream")) {
        YGUI::DrawSliderFloat("DREAM EFFECT", dream, 0.0f, 1.0f);
    }
}

void Settings::DrawSoundTab() {
    // Henuz bir ses motoru yok; degerler simdilik yalnizca CVar'da tutuluyor.
    YGUI::DrawSliderFloat("MASTER VOLUME", &g_CVar.nvs_mastervolume, 0.0f, 1.0f);
    YGUI::DrawSliderFloat("MUSIC VOLUME", &g_CVar.nvs_musicvolume, 0.0f, 1.0f);
}

void Settings::DrawGameplayTab() {
    // Fare hassasiyeti ve yurume sallanmasi client.dll'in kaydettigi degiskenlerdir.
    if (float* sensitivity = ConVarRegistry::FindFloat("cm_sensitivity")) {
        YGUI::DrawSliderFloat("MOUSE SENSITIVITY", sensitivity, 0.02f, 0.5f);
    }
    else {
        YGUI::DrawLabel("client.dll yuklenemedi: fare hassasiyeti ayari kullanilamiyor");
    }

    if (int* bobStyle = ConVarRegistry::FindInt("cm_viewbobstyle")) {
        bool doomBob = (*bobStyle == 1);
        if (YGUI::DrawCheckBox("DOOM VIEW BOB", &doomBob)) {
            *bobStyle = doomBob ? 1 : 0;
        }
    }

    bool doomHud = HUD::doomBarEnabled;
    if (YGUI::DrawCheckBox("DOOM HUD STYLE", &doomHud)) {
        HUD::doomBarEnabled = doomHud;
    }
}

void Settings::Render(int windowWidth, int windowHeight) {
    if (!s_open) return;

    YGUI::SetNextWindowPos(windowWidth * 0.5f, windowHeight * 0.5f, 0.5f, 0.5f);
    YGUI::SetNextWindowSize(WINDOW_WIDTH, 0.0f); // yukseklik icerige gore
    YGUI::DrawWindow("SETTINGS", &s_open);

    static const char* tabNames[static_cast<int>(Category::Count)] = { "GRAPHICS", "SOUND", "GAMEPLAY" };
    for (int i = 0; i < static_cast<int>(Category::Count); i++) {
        if (i > 0) YGUI::SameLine();
        const bool active = (s_activeCategory == static_cast<Category>(i));
        if (YGUI::DrawSelectable(tabNames[i], active, TAB_WIDTH, 0.0f)) {
            s_activeCategory = static_cast<Category>(i);
        }
    }

    YGUI::Separator();
    YGUI::Spacing();

    switch (s_activeCategory) {
    case Category::Graphics: DrawGraphicsTab(); break;
    case Category::Sound:    DrawSoundTab();    break;
    case Category::Gameplay: DrawGameplayTab(); break;
    default: break;
    }

    YGUI::EndWindow();
}