#include "PauseMenu.h"
#include "BitmapFont.h"
#include "GameState.h"
#include "Settings.h"
#include <windows.h>
#include <GL/gl.h>
#include <string>
#include <vector>
#include <functional>

struct PauseEntry {
    std::string label;
    std::function<void()> onClick;
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    bool hovered = false;
};

static std::vector<PauseEntry> s_entries;
static bool s_built = false;

static void BuildEntries() {
    if (s_built) return;
    s_built = true;

    s_entries.push_back({ "RESUME", []() { ResumeFromPause(); } });
    s_entries.push_back({ "SETTINGS", []() { Settings::Open(); } });
    s_entries.push_back({ "QUIT TO MENU", []() { EnterMenuStatic(); } });
}

// Butonlarin ekran konumu, pencere boyutuna gore her frame yeniden hesaplanir.
// Boylece cozunurluk degisse bile liste hep dikey ortalanmis kalir.
static void LayoutEntries(int windowWidth, int windowHeight) {
    const float textHeight = 44.0f;
    const float spacing = 70.0f;

    float totalHeight = spacing * static_cast<float>(s_entries.size() - 1) + textHeight;
    float startY = (static_cast<float>(windowHeight) - totalHeight) * 0.5f;

    for (size_t i = 0; i < s_entries.size(); i++) {
        float w = g_HudFont.MeasureTextWidth(s_entries[i].label, textHeight);
        s_entries[i].x = (static_cast<float>(windowWidth) - w) * 0.5f;
        s_entries[i].y = startY + static_cast<float>(i) * spacing;
        s_entries[i].w = w;
        s_entries[i].h = textHeight;
    }
}

void PauseMenu::Init() {
    BuildEntries();
}

void PauseMenu::HandleMouseMove(int mx, int my) {
    for (auto& e : s_entries) {
        e.hovered = (mx >= e.x && mx <= e.x + e.w && my >= e.y && my <= e.y + e.h);
    }
}

void PauseMenu::HandleMouseClick(int mx, int my) {
    for (auto& e : s_entries) {
        if (mx >= e.x && mx <= e.x + e.w && my >= e.y && my <= e.y + e.h) {
            if (e.onClick) e.onClick();
            return;
        }
    }
}

void PauseMenu::Render(int windowWidth, int windowHeight) {
    BuildEntries();
    LayoutEntries(windowWidth, windowHeight);

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

    // Arkadaki donmus oyun goruntusunu karart.
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f); glVertex2f(static_cast<float>(windowWidth), 0.0f);
    glVertex2f(static_cast<float>(windowWidth), static_cast<float>(windowHeight)); glVertex2f(0.0f, static_cast<float>(windowHeight));
    glEnd();

    for (const auto& e : s_entries) {
        float r = e.hovered ? 1.0f : 0.85f;
        float g = e.hovered ? 0.15f : 0.85f;
        float b = e.hovered ? 0.1f : 0.85f;
        g_HudFont.DrawText(e.x, e.y, e.label, e.h, r, g, b);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}