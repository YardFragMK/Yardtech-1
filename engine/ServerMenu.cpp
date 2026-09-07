#include "ServerMenu.h"
#include "UIWindow.h"
#include "BitmapFont.h"
#include "NetClient.h"
#include "console/Console.h"
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include <string>

namespace {
    bool s_open = false;

    enum class Tab { Internet = 0, Lan, History, Favorite, Count };
    Tab s_activeTab = Tab::Internet;

    struct ServerRow {
        std::string name;
        std::string players; // "current/max" biciminde
        bool anticheat = true;
        bool hovered = false;
    };

    std::vector<ServerRow> s_rows;
    int s_selectedRow = -1;

    constexpr float PANEL_W = 900.0f;
    constexpr float PANEL_H = 560.0f;
    constexpr float TAB_ROW_H = 44.0f;
    constexpr float SIDE_PANEL_W = 200.0f;
    constexpr float JOIN_BTN_H = 46.0f;
    constexpr float ROW_H = 26.0f;

    void FilledRect(float x, float y, float w, float h, float r, float g, float b, float a) {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    }

    void RectOutline(float x, float y, float w, float h, float r, float g, float b, float a) {
        glLineWidth(1.0f);
        glColor4f(r, g, b, a);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    }

    bool PointIn(int mx, int my, float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }

    void BuildPlaceholderRows() {
        if (!s_rows.empty()) return;
        for (int i = 0; i < 9; i++) {
            ServerRow row;
            row.name = "SERVER " + std::to_string(i + 1);
            row.players = std::to_string((i * 3) % 12) + "/16";
            row.anticheat = (i % 2 == 0);
            s_rows.push_back(row);
        }
    }
}

void ServerMenu::Init() {
    BuildPlaceholderRows();
}

void ServerMenu::Open() {
    BuildPlaceholderRows();
    s_open = true;
}

void ServerMenu::Close() {
    s_open = false;
}

bool ServerMenu::IsOpen() {
    return s_open;
}

void ServerMenu::HandleMouseMove(int mx, int my) {
    if (!s_open) return;
    for (auto& row : s_rows) {
        row.hovered = false; // konumlar Render'da hesaplandigi icin gercek hover testi Render icinde de yapilir
    }
    (void)mx; (void)my;
}

void ServerMenu::HandleMouseClick(int mx, int my) {
    if (!s_open) return;

    // Panel/pencere ile ilgili sabit degerler Render ile ayni sekilde
    // hesaplanmali; SDL'den anlik pencere boyutunu tekrar sormak yerine
    // basitlik icin son bilinen boyutu kullanmiyoruz -- Render zaten her
    // karede cagrildigi icin, klik islenirken ayni frame'in konumlarini
    // yeniden hesaplamak guvenlidir.
    int w = 0, h = 0;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);

    float panelX = (w - PANEL_W) * 0.5f;
    float panelY = (h - PANEL_H) * 0.5f;

    if (UIWindow::HandleClick(panelX, panelY, PANEL_W, PANEL_H, mx, my) == UIWindow::Hit::Close) {
        Close();
        return;
    }

    float contentY = UIWindow::ContentStartY(panelY);
    float contentX = UIWindow::ContentStartX(panelX);
    float innerRight = panelX + PANEL_W - UIWindow::GetMargin() - 10.0f;

    // Sekmeler
    const char* tabLabels[(int)Tab::Count] = { "INTERNET", "LAN", "HISTORY", "FAVORITE" };
    float tabX = contentX;
    for (int i = 0; i < (int)Tab::Count; i++) {
        float tabW = g_HudFont.MeasureTextWidth(tabLabels[i], 22.0f) + 30.0f;
        if (PointIn(mx, my, tabX, contentY, tabW, TAB_ROW_H)) {
            s_activeTab = static_cast<Tab>(i);
            return;
        }
        tabX += tabW + 8.0f;
    }

    // Sunucu satirlari
    float listY = contentY + TAB_ROW_H + 15.0f;
    float listW = innerRight - contentX - SIDE_PANEL_W - 15.0f;
    for (size_t i = 0; i < s_rows.size(); i++) {
        float rowY = listY + static_cast<float>(i) * ROW_H;
        if (PointIn(mx, my, contentX, rowY, listW, ROW_H)) {
            s_selectedRow = static_cast<int>(i);
            return;
        }
    }

    // Join server butonu
    float sideX = innerRight - SIDE_PANEL_W;
    float joinY = panelY + PANEL_H - UIWindow::GetMargin() - JOIN_BTN_H - 10.0f;
    if (PointIn(mx, my, sideX, joinY, SIDE_PANEL_W, JOIN_BTN_H)) {
        if (s_selectedRow >= 0 && s_selectedRow < static_cast<int>(s_rows.size())) {
            Console::Log("Baglaniliyor: " + s_rows[s_selectedRow].name + " (henuz gercek adres cozumlemesi yok, sabit test adresi kullaniliyor)");
            NetClient::Connect("127.0.0.1");
            Close();
        }
    }
}

void ServerMenu::Render(int windowWidth, int windowHeight) {
    if (!s_open) return;

    float panelX = (static_cast<float>(windowWidth) - PANEL_W) * 0.5f;
    float panelY = (static_cast<float>(windowHeight) - PANEL_H) * 0.5f;

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

    FilledRect(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 0.0f, 0.0f, 0.55f);

    UIWindow::Draw(panelX, panelY, PANEL_W, PANEL_H, "SERVERS");

    float contentY = UIWindow::ContentStartY(panelY);
    float contentX = UIWindow::ContentStartX(panelX);
    float innerRight = panelX + PANEL_W - UIWindow::GetMargin() - 10.0f;

    // --- Sekmeler ---
    const char* tabLabels[(int)Tab::Count] = { "INTERNET", "LAN", "HISTORY", "FAVORITE" };
    float tabX = contentX;
    for (int i = 0; i < (int)Tab::Count; i++) {
        bool active = (s_activeTab == static_cast<Tab>(i));
        float tabW = g_HudFont.MeasureTextWidth(tabLabels[i], 22.0f) + 30.0f;

        FilledRect(tabX, contentY, tabW, TAB_ROW_H, active ? 0.15f : 0.06f, active ? 0.15f : 0.06f, active ? 0.15f : 0.06f, 1.0f);
        RectOutline(tabX, contentY, tabW, TAB_ROW_H, 0.5f, 0.5f, 0.5f, 0.6f);

        float labelR = active ? 1.0f : 0.75f;
        g_HudFont.DrawText(tabX + 15.0f, contentY + 10.0f, tabLabels[i], 22.0f, labelR, labelR, labelR);

        tabX += tabW + 8.0f;
    }

    // --- Sunucu listesi (sol/orta) ---
    float listY = contentY + TAB_ROW_H + 15.0f;
    float listW = innerRight - contentX - SIDE_PANEL_W - 15.0f;
    float listH = (panelY + PANEL_H - UIWindow::GetMargin() - 10.0f) - listY;

    RectOutline(contentX, listY, listW, listH, 0.4f, 0.4f, 0.4f, 0.6f);

    for (size_t i = 0; i < s_rows.size(); i++) {
        float rowY = listY + static_cast<float>(i) * ROW_H;
        if (rowY + ROW_H > listY + listH) break;

        bool selected = (s_selectedRow == static_cast<int>(i));
        if (selected) {
            FilledRect(contentX, rowY, listW, ROW_H, 0.3f, 0.08f, 0.06f, 0.8f);
        }

        const ServerRow& row = s_rows[i];
        std::string anticheatText = row.anticheat ? "ON" : "OFF";

        g_HudFont.DrawText(contentX + 10.0f, rowY + 4.0f, anticheatText, 16.0f, 0.8f, 0.8f, 0.8f);
        g_HudFont.DrawText(contentX + 130.0f, rowY + 4.0f, row.name, 16.0f, 0.85f, 0.85f, 0.85f);
        g_HudFont.DrawText(contentX + 330.0f, rowY + 4.0f, row.players, 16.0f, 0.7f, 0.7f, 0.7f);
    }

    // --- Sag panel: detay ---
    float sideX = innerRight - SIDE_PANEL_W;
    float detailH = listH - JOIN_BTN_H - 15.0f;

    RectOutline(sideX, listY, SIDE_PANEL_W, detailH, 0.4f, 0.4f, 0.4f, 0.6f);
    g_HudFont.DrawText(sideX + 10.0f, listY + 10.0f, "DETAILS", 18.0f, 0.85f, 0.85f, 0.85f);

    if (s_selectedRow >= 0 && s_selectedRow < static_cast<int>(s_rows.size())) {
        const ServerRow& row = s_rows[s_selectedRow];
        g_HudFont.DrawText(sideX + 10.0f, listY + 45.0f, row.name, 16.0f, 0.8f, 0.8f, 0.8f);
        g_HudFont.DrawText(sideX + 10.0f, listY + 70.0f, row.players, 14.0f, 0.7f, 0.7f, 0.7f);
    }

    // --- Join server butonu ---
    float joinY = listY + detailH + 15.0f;
    bool canJoin = (s_selectedRow >= 0);
    FilledRect(sideX, joinY, SIDE_PANEL_W, JOIN_BTN_H, canJoin ? 0.15f : 0.08f, canJoin ? 0.08f : 0.08f, canJoin ? 0.06f : 0.08f, 1.0f);
    RectOutline(sideX, joinY, SIDE_PANEL_W, JOIN_BTN_H, 0.6f, 0.6f, 0.6f, 0.8f);
    g_HudFont.DrawText(sideX + 30.0f, joinY + 12.0f, "JOIN SERVER", 18.0f, canJoin ? 1.0f : 0.5f, canJoin ? 1.0f : 0.5f, canJoin ? 1.0f : 0.5f);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}