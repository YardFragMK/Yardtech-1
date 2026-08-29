#include "HUD.h"
#include "console/Console.h"
#include "../game/src/Player.h"
#include <windows.h>
#include <GL/gl.h>
#include <string>

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

// Basit cizgi-ikon: art-i (health)
static void DrawCrossIcon(float cx, float cy, float size, float r, float g, float b) {
    glColor4f(r, g, b, 0.9f);
    float t = size * 0.28f; // kalinlik
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

    float baseY = static_cast<float>(windowHeight) - 60.0f;

    // renkler: HL/Black Mesa tarzi amber/turuncu ana renk
    const float amberR = 0.95f, amberG = 0.65f, amberB = 0.15f;
    const float bloodR = 0.85f, bloodG = 0.15f, bloodB = 0.1f;

    bool lowHealth = g_Player.Health < (g_Player.maxHealth / 4);
    float hR = lowHealth ? bloodR : amberR;
    float hG = lowHealth ? bloodG : amberG;
    float hB = lowHealth ? bloodB : amberB;

    // --- SOL ALT: Can + Zirh ---
    float x = 40.0f;

    DrawGlowBehind(x - 5.0f, baseY - 30.0f, 140.0f, 40.0f, hR, hG, hB);
    DrawCrossIcon(x + 14.0f, baseY - 10.0f, 14.0f, hR, hG, hB);
    Console::DrawTextAt(x + 40.0f, baseY - 18.0f, std::to_string(g_Player.Health), hR, hG, hB);

    float armorX = x + 150.0f;
    DrawGlowBehind(armorX - 5.0f, baseY - 30.0f, 140.0f, 40.0f, amberR, amberG, amberB);
    DrawShieldIcon(armorX + 14.0f, armorX == armorX ? baseY - 10.0f : 0.0f, 14.0f, amberR, amberG, amberB);
    Console::DrawTextAt(armorX + 40.0f, baseY - 18.0f, std::to_string(g_Player.Armor), amberR, amberG, amberB);

    // --- SAG ALT: Mermi (sarjordeki / rezerv sarjor sayisi) ---
    int curBullet = g_Player.GetCurrentBullet();
    int maxBullet = g_Player.GetCurrentMaxBullet();
    int curAmmo = g_Player.GetCurrentAmmo();

    std::string bulletText = std::to_string(curBullet);
    std::string reserveText = std::to_string(curAmmo);

    float ammoX = static_cast<float>(windowWidth) - 220.0f;

    DrawGlowBehind(ammoX - 5.0f, baseY - 30.0f, 200.0f, 40.0f, amberR, amberG, amberB);
    DrawBulletIcon(ammoX + 12.0f, baseY - 6.0f, 14.0f, amberR, amberG, amberB);
    Console::DrawTextAt(ammoX + 36.0f, baseY - 18.0f, bulletText, amberR, amberG, amberB);
    Console::DrawTextAt(ammoX + 36.0f + static_cast<float>(bulletText.size()) * 9.0f + 6.0f,
        baseY - 18.0f, "/", 0.5f, 0.5f, 0.5f);
    Console::DrawTextAt(ammoX + 36.0f + static_cast<float>(bulletText.size()) * 9.0f + 20.0f,
        baseY - 18.0f, reserveText, 0.6f, 0.6f, 0.6f);
    (void)maxBullet;

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}