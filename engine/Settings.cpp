#include "Settings.h"
#include "BitmapFont.h"
#include "Camera.h"
#include "console/CVar.h"
#include "renderer/Renderer.h"
#include "HUD.h"

extern Renderer renderer; // Engine.cpp'de tanimli global

bool Settings::s_open = false;
bool Settings::s_built = false;
Settings::Category Settings::s_activeCategory = Settings::Category::Graphics;

std::vector<Settings::SliderControl> Settings::s_sliders[(int)Category::Count];
std::vector<Settings::ToggleControl> Settings::s_toggles[(int)Category::Count];

float Settings::s_panelX = 0.0f;
float Settings::s_panelY = 0.0f;
float Settings::s_panelW = 820.0f;
float Settings::s_panelH = 560.0f;

void Settings::BuildControls() {
    if (s_built) return;
    s_built = true;

    // --- GRAPHICS ---
    s_toggles[(int)Category::Graphics].push_back({
        "RETRO MODE",
        []() { return renderer.IsRetroMode(); },
        [](bool v) { renderer.SetRetroMode(v); },
        40.0f, 130.0f
        });

    // --- SOUND ---
    // Henuz bir ses motoru yok; bu degerler simdilik sadece CVar'da tutuluyor,
    // ses sistemi eklendiginde dogrudan buraya baglanabilir.
    s_sliders[(int)Category::Sound].push_back({
        "MASTER VOLUME", &g_CVar.nvs_mastervolume, 0.0f, 1.0f, 40.0f, 150.0f, 320.0f
        });
    s_sliders[(int)Category::Sound].push_back({
        "MUSIC VOLUME", &g_CVar.nvs_musicvolume, 0.0f, 1.0f, 40.0f, 240.0f, 320.0f
        });

    // --- GAMEPLAY ---
    s_sliders[(int)Category::Gameplay].push_back({
        "MOUSE SENSITIVITY", &g_Camera.mouseSensitivity, 0.02f, 0.5f, 40.0f, 130.0f, 320.0f
        });
    s_toggles[(int)Category::Gameplay].push_back({
        "DOOM VIEW BOB",
        []() { return g_Camera.viewBobStyle == 1; },
        [](bool v) { g_Camera.viewBobStyle = v ? 1 : 0; },
        40.0f, 220.0f
        });
    s_toggles[(int)Category::Gameplay].push_back({
        "DOOM HUD STYLE",
        []() { return HUD::doomBarEnabled; },
        [](bool v) { HUD::doomBarEnabled = v; },
        40.0f, 270.0f
        });
}

void Settings::Open() {
    BuildControls();
    s_open = true;
}

void Settings::Close() {
    s_open = false;
}

bool Settings::IsOpen() {
    return s_open;
}

static bool PointInRect(int px, int py, float x, float y, float w, float h) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

void Settings::HandleMouseMove(int mx, int my) {
    if (!s_open) return;

    for (auto& s : s_sliders[(int)s_activeCategory]) {
        if (!s.dragging) continue;
        float barX = s_panelX + s.dx;
        float t = (static_cast<float>(mx) - barX) / s.w;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        *s.valuePtr = s.minVal + t * (s.maxVal - s.minVal);
    }
}

void Settings::HandleMouseDown(int mx, int my) {
    if (!s_open) return;

    // Kategori sekmeleri
    const char* tabLabels[(int)Category::Count] = { "GRAPHICS", "VIDEO", "SOUND", "GAMEPLAY" };
    float tabX = s_panelX + 30.0f;
    float tabY = s_panelY + 25.0f;
    for (int i = 0; i < (int)Category::Count; i++) {
        float w = g_HudFont.MeasureTextWidth(tabLabels[i], 24.0f);
        if (PointInRect(mx, my, tabX, tabY, w, 30.0f)) {
            s_activeCategory = static_cast<Category>(i);
            return;
        }
        tabX += w + 40.0f;
    }

    // Aktif kategorideki slider/toggle'lar
    for (auto& s : s_sliders[(int)s_activeCategory]) {
        float barX = s_panelX + s.dx;
        float barY = s_panelY + s.dy;
        if (PointInRect(mx, my, barX, barY - 8.0f, s.w, 24.0f)) {
            s.dragging = true;
            float t = (static_cast<float>(mx) - barX) / s.w;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            *s.valuePtr = s.minVal + t * (s.maxVal - s.minVal);
        }
    }

    for (auto& t : s_toggles[(int)s_activeCategory]) {
        float lx = s_panelX + t.dx;
        float ly = s_panelY + t.dy;
        std::string current = t.label + "   " + (t.getValue() ? "ON" : "OFF");
        float w = g_HudFont.MeasureTextWidth(current, 22.0f);
        if (PointInRect(mx, my, lx, ly, w, 28.0f)) {
            t.setValue(!t.getValue());
            return;
        }
    }
}

void Settings::HandleMouseUp() {
    for (auto& list : s_sliders) {
        for (auto& s : list) s.dragging = false;
    }
}

static void DrawFilledRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

void Settings::RenderCategoryTabs() {
    const char* tabLabels[(int)Category::Count] = { "GRAPHICS", "VIDEO", "SOUND", "GAMEPLAY" };
    float tabX = s_panelX + 30.0f;
    float tabY = s_panelY + 25.0f;

    for (int i = 0; i < (int)Category::Count; i++) {
        bool active = (s_activeCategory == static_cast<Category>(i));
        float r = active ? 1.0f : 0.6f;
        float g = active ? 0.15f : 0.6f;
        float b = active ? 0.1f : 0.6f;
        float w = g_HudFont.DrawText(tabX, tabY, tabLabels[i], 24.0f, r, g, b);
        tabX += w + 40.0f;
    }
}

void Settings::RenderActiveCategory() {
    for (const auto& s : s_sliders[(int)s_activeCategory]) {
        float labelX = s_panelX + s.dx;
        float labelY = s_panelY + s.dy - 34.0f;
        g_HudFont.DrawText(labelX, labelY, s.label, 20.0f, 0.85f, 0.85f, 0.85f);

        float barX = s_panelX + s.dx;
        float barY = s_panelY + s.dy;
        DrawFilledRect(barX, barY, s.w, 6.0f, 0.25f, 0.25f, 0.28f, 1.0f);

        float t = (*s.valuePtr - s.minVal) / (s.maxVal - s.minVal);
        float handleX = barX + t * s.w;
        DrawFilledRect(handleX - 6.0f, barY - 8.0f, 12.0f, 22.0f, 0.85f, 0.15f, 0.1f, 1.0f);

        int displayValue = static_cast<int>(t * 100.0f + 0.5f);
        g_HudFont.DrawText(barX + s.w + 20.0f, barY - 10.0f, std::to_string(displayValue), 20.0f, 0.7f, 0.7f, 0.7f);
    }

    for (const auto& t : s_toggles[(int)s_activeCategory]) {
        float lx = s_panelX + t.dx;
        float ly = s_panelY + t.dy;
        bool val = t.getValue();

        float labelW = g_HudFont.DrawText(lx, ly, t.label, 22.0f, 0.85f, 0.85f, 0.85f);

        float stateR = val ? 0.3f : 0.5f;
        float stateG = val ? 0.85f : 0.5f;
        float stateB = val ? 0.3f : 0.5f;
        g_HudFont.DrawText(lx + labelW + 30.0f, ly, val ? "ON" : "OFF", 22.0f, stateR, stateG, stateB);
    }
}

void Settings::Render(int windowWidth, int windowHeight) {
    if (!s_open) return;

    s_panelX = (static_cast<float>(windowWidth) - s_panelW) * 0.5f;
    s_panelY = (static_cast<float>(windowHeight) - s_panelH) * 0.5f;

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

    // Panelin arkasinda hafif karartma (altindaki menu/oyun goruntusunu soldurur)
    DrawFilledRect(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight),
        0.0f, 0.0f, 0.0f, 0.55f);

    // Ana panel: siyah dolgu
    DrawFilledRect(s_panelX, s_panelY, s_panelW, s_panelH, 0.05f, 0.05f, 0.06f, 0.97f);

    RenderCategoryTabs();

    // Sekmelerin altindaki ince kirmizi serit
    DrawFilledRect(s_panelX + 20.0f, s_panelY + 68.0f, s_panelW - 40.0f, 3.0f, 0.75f, 0.1f, 0.08f, 1.0f);

    RenderActiveCategory();

    // Panel kenarligi
    glLineWidth(2.0f);
    glColor4f(0.5f, 0.1f, 0.08f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(s_panelX, s_panelY);
    glVertex2f(s_panelX + s_panelW, s_panelY);
    glVertex2f(s_panelX + s_panelW, s_panelY + s_panelH);
    glVertex2f(s_panelX, s_panelY + s_panelH);
    glEnd();
    glLineWidth(1.0f);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}