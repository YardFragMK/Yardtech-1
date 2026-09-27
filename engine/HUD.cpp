#include "HUD.h"
#include "BitmapFont.h"
#include "../game/src/Player.h"
#include <windows.h>
#include <GL/gl.h>
#include <string>

bool HUD::doomBarEnabled = false;

// Bir sayinin arkasina yumusak, katmanli bir parlama (glow) cizer.
// Gercek blur yok (FFP), bunun yerine buyuyen, giderek soluklasan yarim-saydam
// dikdortgenler ust uste binerek Black Mesa tarzi retro-glow hissi verir.
static void DrawGlowBehind(float x, float y, float w, float h, float r, float g, float b) {
    const int layers = 4;
    for (int i = layers; i >= 1; i--) {
        float pad = i * 4.0f;
        float alpha = 0.10f / i;

    }
}

// Basit cizgi-ikon: arti (health)
static void DrawCrossIcon(float cx, float cy, float size, float r, float g, float b) {

    float t = size * 0.28f;

}

// Basit cizgi-ikon: kalkan (armor) -- besgen benzeri basit sekil
static void DrawShieldIcon(float cx, float cy, float size, float r, float g, float b) {

}

// Basit cizgi-ikon: mermi (dikdortgen govde + ucu sivri)
static void DrawBulletIcon(float cx, float cy, float size, float r, float g, float b) {

}

// --- Doom-tarzi siyah-beyaz retro bar icin yardimcilar ---

static void DrawRetroPanel(float x, float y, float w, float h) {

}

static void DrawRetroDivider(float x, float y0, float y1) {

}

static void RenderDoomBar(int windowWidth, int windowHeight) {
    const float barHeight = 90.0f;
    float barY = static_cast<float>(windowHeight) - barHeight;
    float barW = static_cast<float>(windowWidth);

    DrawRetroPanel(0.0f, barY, barW, barHeight);

    float third = barW / 3.0f;
    DrawRetroDivider(third, barY + 8.0f, barY + barHeight - 8.0f);
    DrawRetroDivider(third * 2.0f, barY + 8.0f, barY + barHeight - 8.0f);

    float iconY = barY + barHeight * 0.5f;
    const float labelHeight = 18.0f;
    const float numberHeight = 30.0f;

    // --- HEALTH (sol) ---
    DrawCrossIcon(third * 0.25f, iconY, 16.0f, 1.0f, 1.0f, 1.0f);
    g_HudFont.UIDrawText(third * 0.25f + 30.0f, barY + 14.0f, "HP", labelHeight, 1.0f, 1.0f, 1.0f);
    g_HudFont.UIDrawText(third * 0.25f + 30.0f, barY + 36.0f, std::to_string(g_Player.Health), numberHeight, 1.0f, 1.0f, 1.0f);

    // --- ARMOR (orta) ---
    float armorCx = third + third * 0.25f;
    DrawShieldIcon(armorCx, iconY, 16.0f, 1.0f, 1.0f, 1.0f);
    g_HudFont.UIDrawText(armorCx + 30.0f, barY + 14.0f, "AP", labelHeight, 1.0f, 1.0f, 1.0f);
    g_HudFont.UIDrawText(armorCx + 30.0f, barY + 36.0f, std::to_string(g_Player.Armor), numberHeight, 1.0f, 1.0f, 1.0f);

    // --- AMMO (sag) ---
    int curBullet = g_Player.GetCurrentBullet();
    int curAmmo = g_Player.GetCurrentAmmo();
    float ammoCx = third * 2.0f + third * 0.25f;

    DrawBulletIcon(ammoCx, iconY, 16.0f, 1.0f, 1.0f, 1.0f);
    g_HudFont.UIDrawText(ammoCx + 30.0f, barY + 14.0f, "AMMO", labelHeight, 1.0f, 1.0f, 1.0f);

    float ammoNumX = ammoCx + 30.0f;
    float ammoNumY = barY + 36.0f;
    ammoNumX += g_HudFont.UIDrawText(ammoNumX, ammoNumY, std::to_string(curBullet), numberHeight, 1.0f, 1.0f, 1.0f);
    ammoNumX += g_HudFont.UIDrawText(ammoNumX, ammoNumY, "/", numberHeight, 0.7f, 0.7f, 0.7f);
    g_HudFont.UIDrawText(ammoNumX, ammoNumY, std::to_string(curAmmo), numberHeight, 0.7f, 0.7f, 0.7f);
}

void HUD::Render(int windowWidth, int windowHeight) {

    if (doomBarEnabled) {
        RenderDoomBar(windowWidth, windowHeight);
    }
    else {
        // --- mevcut HUD, artik sadece ikon + yazi (glow/dikdortgen yok) ---
        float baseY = static_cast<float>(windowHeight) - 60.0f;
        const float numberHeight = 34.0f;

        //const float amberR = 0.95f, amberG = 0.65f, amberB = 0.15f;
        //const float bloodR = 0.85f, bloodG = 0.15f, bloodB = 0.1f;
        const float amberR = 0.5f, amberG = 0.5f, amberB = 0.5f;
        const float bloodR = 0.5f, bloodG = 0.5f, bloodB =0.5f;
        bool lowHealth = g_Player.Health < (g_Player.maxHealth / 4);
        float hR = lowHealth ? bloodR : amberR;
        float hG = lowHealth ? bloodG : amberG;
        float hB = lowHealth ? bloodB : amberB;

        // --- SOL ALT: Can + Zirh ---
        float x = 40.0f;

        DrawCrossIcon(x + 14.0f, baseY - 10.0f, 14.0f, hR, hG, hB);
        g_HudFont.UIDrawText(x + 40.0f, baseY - 30.0f, std::to_string(g_Player.Health), numberHeight, hR, hG, hB);

        float armorX = x + 150.0f;
        DrawShieldIcon(armorX + 14.0f, baseY - 10.0f, 14.0f, amberR, amberG, amberB);
        g_HudFont.UIDrawText(armorX + 40.0f, baseY - 30.0f, std::to_string(g_Player.Armor), numberHeight, amberR, amberG, amberB);

        // --- SAG ALT: Mermi (sarjordeki / rezerv sarjor sayisi) ---
        int curBullet = g_Player.GetCurrentBullet();
        int maxBullet = g_Player.GetCurrentMaxBullet();
        int curAmmo = g_Player.GetCurrentAmmo();

        float ammoX = static_cast<float>(windowWidth) - 220.0f;
        float ammoY = baseY - 30.0f;

        DrawBulletIcon(ammoX + 12.0f, baseY - 6.0f, 14.0f, amberR, amberG, amberB);

        float cursorX = ammoX + 36.0f;
        cursorX += g_HudFont.UIDrawText(cursorX, ammoY, std::to_string(curBullet), numberHeight, amberR, amberG, amberB);
        cursorX += g_HudFont.UIDrawText(cursorX, ammoY, "+", numberHeight, 0.5f, 0.5f, 0.5f);
        g_HudFont.UIDrawText(cursorX, ammoY, std::to_string(curAmmo), numberHeight, 0.6f, 0.6f, 0.6f);

        (void)maxBullet;
    }

}