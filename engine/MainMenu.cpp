#include "MainMenu.h"
#include "TGALoader.h"
#include "BitmapFont.h"
#include "Camera.h"
#include "GameState.h"
#include <cmath>
#include "GLExtensions.h"

GLuint MainMenu::s_bgTexture = 0;
std::vector<MenuButton> MainMenu::s_buttons;
float MainMenu::s_liveCamYaw = 0.0f;

bool MainMenu::LoadBackgroundImage(const std::string& tgaPath) {
    std::vector<uint8_t> pixels;
    int w = 0, h = 0;
    if (!LoadTGA(tgaPath, pixels, w, h)) return false;

    if (s_bgTexture != 0) glDeleteTextures(1, &s_bgTexture);

    glGenTextures(1, &s_bgTexture);
    glBindTexture(GL_TEXTURE_2D, s_bgTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void MainMenu::Init() {
    // Bilerek bos: buton yerlesimini Engine::initSystems() icinde
    // AddButton ile kendin kuruyorsun.
}

void MainMenu::ClearButtons() {
    s_buttons.clear();
}

void MainMenu::AddButton(float x, float y, float w, float h, const std::string& label, std::function<void()> onClick) {
    MenuButton b;
    b.x = x; b.y = y; b.w = w; b.h = h;
    b.label = label;
    b.onClick = onClick;
    s_buttons.push_back(b);
}

void MainMenu::HandleMouseMove(int mx, int my) {
    for (auto& b : s_buttons) {
        b.hovered = (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h);
    }
}

void MainMenu::HandleMouseClick(int mx, int my) {
    for (auto& b : s_buttons) {
        if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h) {
            if (b.onClick) b.onClick();
            return;
        }
    }
}

void MainMenu::UpdateLiveCamera(float deltaTime) {
    // Yavasca donen, sabit bir noktada duran "idle" kamera -- haritayi sergiler.
    // g_Camera.position, harita yuklenirken (player_start ya da senin ayarladigin
    // sabit bir menu-kamera noktasi) zaten dogru yerde olmali.
    const float ORBIT_SPEED = 6.0f;
    s_liveCamYaw += ORBIT_SPEED * deltaTime;
    if (s_liveCamYaw > 360.0f) s_liveCamYaw -= 360.0f;

    g_Camera.yaw = s_liveCamYaw;
    g_Camera.pitch = -5.0f;
}

void MainMenu::Update(float deltaTime) {
    if (g_State == GameState::MenuLive) {
        UpdateLiveCamera(deltaTime);
    }
}

static void DrawFullscreenQuad(GLuint tex, int windowWidth, int windowHeight) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f((float)windowWidth, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f((float)windowWidth, (float)windowHeight);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)windowHeight);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// Buton icin sadece bitmap font ile buyuk metin ciziyor -- dikdortgen/kenarlik yok.
// Hover alani hala AddButton'daki x/y/w/h dikdortgenine gore hesaplaniyor,
// sadece gorsel olarak gorunmuyor.
static void DrawButtonText(const MenuButton& b, float pixelHeight) {
    float r = b.hovered ? 1.0f : 0.85f;
    float g = b.hovered ? 0.15f : 0.85f;
    float bl = b.hovered ? 0.1f : 0.85f;

    g_HudFont.DrawText(b.x, b.y, b.label, pixelHeight, r, g, bl);
}

void MainMenu::Render(int windowWidth, int windowHeight) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (g_State == GameState::MenuStatic) {
        if (s_bgTexture != 0) {
            DrawFullscreenQuad(s_bgTexture, windowWidth, windowHeight);
        }
        else {
            glDisable(GL_TEXTURE_2D);
            glColor4f(0.05f, 0.05f, 0.06f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f((float)windowWidth, 0);
            glVertex2f((float)windowWidth, (float)windowHeight); glVertex2f(0, (float)windowHeight);
            glEnd();
        }
    }
    // MenuLive: arkaplan zaten 3D dunya (RenderFrame icinde ayrica ciziliyor), burada sadece butonlar.

    const float BUTTON_TEXT_HEIGHT = 46.0f; // piksel yukseklik, buradan ayarlanir
    for (const auto& b : s_buttons) {
        DrawButtonText(b, BUTTON_TEXT_HEIGHT);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}