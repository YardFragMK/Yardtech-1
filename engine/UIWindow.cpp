#include "UIWindow.h"
#include "TGALoader.h"
#include "BitmapFont.h"
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include "GLExtensions.h"

unsigned int UIWindow::s_iconTexture = 0;

namespace {
    constexpr float MARGIN = 10.0f;
    constexpr float TITLE_ROW_HEIGHT = 34.0f;
    constexpr float ICON_SIZE = 22.0f;
    constexpr float CLOSE_SIZE = 18.0f;
    constexpr float INNER_PADDING = 10.0f;

    void FilledRect(float x, float y, float w, float h, float r, float g, float b, float a) {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    }

    void RectOutline(float x, float y, float w, float h, float r, float g, float b, float a, float lineW) {
        glLineWidth(lineW);
        glColor4f(r, g, b, a);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
        glLineWidth(1.0f);
    }

    void DrawCloseGlyph(float cx, float cy, float size, float r, float g, float b) {
        glLineWidth(2.0f);
        glColor4f(r, g, b, 1.0f);
        glBegin(GL_LINES);
        glVertex2f(cx - size, cy - size); glVertex2f(cx + size, cy + size);
        glVertex2f(cx - size, cy + size); glVertex2f(cx + size, cy - size);
        glEnd();
        glLineWidth(1.0f);
    }

    bool PointIn(int mx, int my, float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }

    void GetInnerRect(float x, float y, float w, float h, float& ix, float& iy, float& iw, float& ih) {
        ix = x + MARGIN;
        iy = y + MARGIN;
        iw = w - MARGIN * 2.0f;
        ih = h - MARGIN * 2.0f;
    }
}

bool UIWindow::GBLoadIcon(const std::string& iconTgaPath) {
    std::vector<uint8_t> pixels;
    int w = 0, h = 0;
    if (!LoadTGA(iconTgaPath, pixels, w, h)) return false;

    glGenTextures(1, &s_iconTexture);
    glBindTexture(GL_TEXTURE_2D, s_iconTexture);
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

void UIWindow::Draw(float x, float y, float w, float h, const std::string& title) {
    const float borderR = 0.78f, borderG = 0.75f, borderB = 0.75f;
    const float accentR = 0.75f, accentG = 0.08f, accentB = 0.06f;

    // Dis kutu: kenarliksiz, dolgu siyah.
    FilledRect(x, y, w, h, 0.02f, 0.02f, 0.02f, 0.98f);

    float ix, iy, iw, ih;
    GetInnerRect(x, y, w, h, ix, iy, iw, ih);

    // Ic kutu: sadece beyaz kenarlik, icerik bunun icinde durur.
    RectOutline(ix, iy, iw, ih, borderR, borderG, borderB, 0.9f, 2.0f);

    float iconX = ix + INNER_PADDING;
    float iconY = iy + (TITLE_ROW_HEIGHT - ICON_SIZE) * 0.5f;

    if (s_iconTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, s_iconTexture);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(iconX, iconY);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(iconX + ICON_SIZE, iconY);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(iconX + ICON_SIZE, iconY + ICON_SIZE);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(iconX, iconY + ICON_SIZE);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    float titleX = iconX + (s_iconTexture != 0 ? ICON_SIZE + 10.0f : 0.0f);
    float titleY = iy + (TITLE_ROW_HEIGHT - 20.0f) * 0.5f;
    g_HudFont.DrawText(titleX, titleY, title, 20.0f, 0.92f, 0.9f, 0.9f);

    FilledRect(ix + INNER_PADDING, iy + TITLE_ROW_HEIGHT, iw - INNER_PADDING * 2.0f, 2.0f, accentR, accentG, accentB, 1.0f);

    float closeX = ix + iw - INNER_PADDING - CLOSE_SIZE;
    float closeY = iy + (TITLE_ROW_HEIGHT - CLOSE_SIZE) * 0.5f;
    RectOutline(closeX, closeY, CLOSE_SIZE, CLOSE_SIZE, borderR, borderG, borderB, 0.85f, 1.0f);
    DrawCloseGlyph(closeX + CLOSE_SIZE * 0.5f, closeY + CLOSE_SIZE * 0.5f, 5.0f, 0.9f, 0.3f, 0.25f);
}

UIWindow::Hit UIWindow::HandleClick(float x, float y, float w, float h, int mx, int my) {
    float ix, iy, iw, ih;
    GetInnerRect(x, y, w, h, ix, iy, iw, ih);

    float closeX = ix + iw - INNER_PADDING - CLOSE_SIZE;
    float closeY = iy + (TITLE_ROW_HEIGHT - CLOSE_SIZE) * 0.5f;

    if (PointIn(mx, my, closeX, closeY, CLOSE_SIZE, CLOSE_SIZE)) return Hit::Close;
    return Hit::None;
}

float UIWindow::ContentStartY(float y) {
    return y + MARGIN + TITLE_ROW_HEIGHT + 8.0f;
}

float UIWindow::ContentStartX(float x) {
    return x + MARGIN + INNER_PADDING;
}

float UIWindow::GetMargin() {
    return MARGIN;
}