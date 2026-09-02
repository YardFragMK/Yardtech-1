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
        glColor4f(r, g, b, alpha);
        glBegin(GL_QUADS);
        glVertex2f(x - pad, y - pad);
        glVertex2f(x + w + pad, y - pad);
        glVertex2f(x + w + pad, y + h + pad);
        glVertex2f(x - pad, y + h + pad);
        glEnd();
    }
}

// Basit cizgi-ikon: arti (health)
static void DrawCrossIcon(float cx, float cy, float size, float r, float g, float b) {
    glColor4f(r, g, b, 0.9f);
    float t = size * 0.28f;
    glBegin(GL_QUADS);
    glVertex2f(cx - t, cy - size); glVertex2f(cx + t, cy - size);
    glVertex2f(cx + t, cy + size); glVertex2f(cx - t, cy + size);
    glVertex2f(cx - size, cy - t); glVertex2f(cx + size, cy - t);
    glVertex2f(cx + size, cy + t); glVertex2f(cx - size, cy + t);
    glEnd();
}

// Basit cizgi-ikon: kalkan (armor) -- besgen benzeri basit sekil
static void DrawShieldIcon(float cx, float cy, float size, float r, float g, float b) {
    glColor4f(r, g, b, 0.9f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy - size);
    glVertex2f(cx - size, cy - size * 0.3f);
    glVertex2f(cx - size * 0.6f, cy + size * 0.7f);
    glVertex2f(cx, cy + size);
    glVertex2f(cx + size * 0.6f, cy + size * 0.7f);
    glVertex2f(cx + size, cy - size * 0.3f);
    glEnd();
}

// Basit cizgi-ikon: mermi (dikdortgen govde + ucu sivri)
static void DrawBulletIcon(float cx, float cy, float size, float r, float g, float b) {
    glColor4f(r, g, b, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(cx - size * 0.35f, cy - size * 0.2f);
    glVertex2f(cx + size * 0.15f, cy - size * 0.2f);
    glVertex2f(cx + size * 0.15f, cy + size);
    glVertex2f(cx - size * 0.35f, cy + size);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - size * 0.35f, cy - size * 0.2f);
    glVertex2f(cx + size * 0.15f, cy - size * 0.2f);
    glVertex2f(cx - size * 0.1f, cy - size);
    glEnd();
}

// --- Doom-tarzi siyah-beyaz retro bar icin yardimcilar ---

static void DrawRetroPanel(float x, float y, float w, float h) {
    glColor4f(0.0f, 0.0f, 0.0f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();

    glLineWidth(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);
}

static void DrawRetroDivider(float x, float y0, float y1) {
    glLineWidth(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glBegin(GL_LINES);
    glVertex2f(x, y0);
    glVertex2f(x, y1);
    glEnd();
    glLineWidth(1.0f);
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
    g_HudFont.DrawText(third * 0.25f + 30.0f, barY + 14.0f, "HP", labelHeight, 1.0f, 1.0f, 1.0f);
    g_HudFont.DrawText(third * 0.25f + 30.0f, barY + 36.0f, std::to_string(g_Player.Health), numberHeight, 1.0f, 1.0f, 1.0f);

    // --- ARMOR (orta) ---
    float armorCx = third + third * 0.25f;
    DrawShieldIcon(armorCx, iconY, 16.0f, 1.0f, 1.0f, 1.0f);
    g_HudFont.DrawText(armorCx + 30.0f, barY + 14.0f, "AP", labelHeight, 1.0f, 1.0f, 1.0f);
    g_HudFont.DrawText(armorCx + 30.0f, barY + 36.0f, std::to_string(g_Player.Armor), numberHeight, 1.0f, 1.0f, 1.0f);

    // --- AMMO (sag) ---
    int curBullet = g_Player.GetCurrentBullet();
    int curAmmo = g_Player.GetCurrentAmmo();
    float ammoCx = third * 2.0f + third * 0.25f;

    DrawBulletIcon(ammoCx, iconY, 16.0f, 1.0f, 1.0f, 1.0f);
    g_HudFont.DrawText(ammoCx + 30.0f, barY + 14.0f, "AMMO", labelHeight, 1.0f, 1.0f, 1.0f);

    float ammoNumX = ammoCx + 30.0f;
    float ammoNumY = barY + 36.0f;
    ammoNumX += g_HudFont.DrawText(ammoNumX, ammoNumY, std::to_string(curBullet), numberHeight, 1.0f, 1.0f, 1.0f);
    ammoNumX += g_HudFont.DrawText(ammoNumX, ammoNumY, "/", numberHeight, 0.7f, 0.7f, 0.7f);
    g_HudFont.DrawText(ammoNumX, ammoNumY, std::to_string(curAmmo), numberHeight, 0.7f, 0.7f, 0.7f);
}

void HUD::Render(int windowWidth, int windowHeight) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        g_HudFont.DrawText(x + 40.0f, baseY - 30.0f, std::to_string(g_Player.Health), numberHeight, hR, hG, hB);

        float armorX = x + 150.0f;
        DrawShieldIcon(armorX + 14.0f, baseY - 10.0f, 14.0f, amberR, amberG, amberB);
        g_HudFont.DrawText(armorX + 40.0f, baseY - 30.0f, std::to_string(g_Player.Armor), numberHeight, amberR, amberG, amberB);

        // --- SAG ALT: Mermi (sarjordeki / rezerv sarjor sayisi) ---
        int curBullet = g_Player.GetCurrentBullet();
        int maxBullet = g_Player.GetCurrentMaxBullet();
        int curAmmo = g_Player.GetCurrentAmmo();

        float ammoX = static_cast<float>(windowWidth) - 220.0f;
        float ammoY = baseY - 30.0f;

        DrawBulletIcon(ammoX + 12.0f, baseY - 6.0f, 14.0f, amberR, amberG, amberB);

        float cursorX = ammoX + 36.0f;
        cursorX += g_HudFont.DrawText(cursorX, ammoY, std::to_string(curBullet), numberHeight, amberR, amberG, amberB);
        cursorX += g_HudFont.DrawText(cursorX, ammoY, "+", numberHeight, 0.5f, 0.5f, 0.5f);
        g_HudFont.DrawText(cursorX, ammoY, std::to_string(curAmmo), numberHeight, 0.6f, 0.6f, 0.6f);

        (void)maxBullet;
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}