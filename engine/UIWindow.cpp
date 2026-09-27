#include "UIWindow.h"
#include "TGALoader.h"
#include "BitmapFont.h"
#include <windows.h>
#include <vector>
#include "GLExtensions.h"
#include "RTInstance.h"
#include "RTUI.h"

std::string UIWindow::s_iconTextureName = "";

namespace {
    constexpr float MARGIN = 10.0f;
    constexpr float TITLE_ROW_HEIGHT = 34.0f;
    constexpr float ICON_SIZE = 22.0f;
    constexpr float CLOSE_SIZE = 18.0f;
    constexpr float INNER_PADDING = 10.0f;

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

    RgOriginalTextureInfo texInfo{};
    texInfo.pTextureName = "ui_window_icon";
    texInfo.pPixels = pixels.data();
    texInfo.size = RgExtent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    texInfo.filter = RG_SAMPLER_FILTER_LINEAR;

    rgProvideOriginalTexture(GetRTInstance(), &texInfo);
    s_iconTextureName = "ui_window_icon";
    return true;
}

void UIWindow::Draw(float x, float y, float w, float h, const std::string& title) {
    const float borderR = 0.78f, borderG = 0.75f, borderB = 0.75f;
    const float accentR = 0.75f, accentG = 0.08f, accentB = 0.06f;

    // Dis kutu: kenarliksiz, dolgu siyah.
    RTUI::FilledRect(x, y, w, h, 0.02f, 0.02f, 0.02f, 0.98f);

    float ix, iy, iw, ih;
    GetInnerRect(x, y, w, h, ix, iy, iw, ih);

    // Ic kutu: sadece beyaz kenarlik, icerik bunun icinde durur.
    RTUI::RectOutline(ix, iy, iw, ih, borderR, borderG, borderB, 0.9f, 2.0f);

    float iconX = ix + INNER_PADDING;
    float iconY = iy + (TITLE_ROW_HEIGHT - ICON_SIZE) * 0.5f;

    if (!s_iconTextureName.empty()) {
        RTUI::TexturedQuad(iconX, iconY, ICON_SIZE, ICON_SIZE, s_iconTextureName, 0, 0, 1, 1, 1, 1, 1, 1);
    }

    float titleX = iconX + (!s_iconTextureName.empty() ? ICON_SIZE + 10.0f : 0.0f);
    float titleY = iy + (TITLE_ROW_HEIGHT - 20.0f) * 0.5f;
    g_HudFont.UIDrawText(titleX, titleY, title, 20.0f, 0.92f, 0.9f, 0.9f);

    RTUI::FilledRect(ix + INNER_PADDING, iy + TITLE_ROW_HEIGHT, iw - INNER_PADDING * 2.0f, 2.0f, accentR, accentG, accentB, 1.0f);

    float closeX = ix + iw - INNER_PADDING - CLOSE_SIZE;
    float closeY = iy + (TITLE_ROW_HEIGHT - CLOSE_SIZE) * 0.5f;
    RTUI::RectOutline(closeX, closeY, CLOSE_SIZE, CLOSE_SIZE, borderR, borderG, borderB, 0.85f, 1.0f);
    float offset = 5.0f; 
    RTUI::Line(closeX + offset, closeY + offset, closeX + CLOSE_SIZE - offset, closeY + CLOSE_SIZE - offset, borderR, borderG, borderB, 0.85f, 1.5f);
    RTUI::Line(closeX + CLOSE_SIZE - offset, closeY + offset, closeX + offset, closeY + CLOSE_SIZE - offset, borderR, borderG, borderB, 0.85f, 1.5f);
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