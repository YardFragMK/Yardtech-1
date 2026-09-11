#include "ServerMenu.h"
#include "UIWindow.h"
#include "BitmapFont.h"
#include "NetClient.h"
#include "console/Console.h"
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include <string>
#include <algorithm>

namespace {
    bool s_open = false;

    enum class Tab { Internet = 0, Lan, History, Favorite, Count };
    Tab s_activeTab = Tab::Internet;

    struct ServerRow {
        std::string name;
        std::string mapName;
        std::string ipAddress; // gercek kesif gelene kadar elle tanimli test adresleri
        int currentPlayers = 0;
        int maxPlayers = 16;
        bool hasPassword = false;
        bool hasAnticheat = true;
    };

    // Her sekmenin kendi listesi. Internet/Lan ornek veriyle dolu; History
    // ve Favorite bos -- sekme gecisinin gercekten calistigini gormek icin
    // bos bir liste de anlamli bir durumdur ("henuz kayit yok" mesaji).
    std::vector<ServerRow> s_rowsByTab[(int)Tab::Count];

    int s_selectedRow = -1;
    int s_scrollOffset = 0; // ilk gorunen satirin indeksi

    constexpr float PANEL_W = 920.0f;
    constexpr float PANEL_H = 580.0f;
    constexpr float TAB_ROW_H = 40.0f;
    constexpr float HEADER_ROW_H = 26.0f;
    constexpr float SIDE_PANEL_W = 200.0f;
    constexpr float JOIN_BTN_H = 46.0f;
    constexpr float ROW_H = 28.0f;
    constexpr int SCROLL_STEP = 1;

    // Sutun x-ofsetleri, listenin sol kenarina (contentX) gore. Baslik
    // satiriyla veri satirlari bu sabitler uzerinden hizalanir.
    constexpr float COL_LOCK = 8.0f;
    constexpr float COL_ANTICHEAT = 40.0f;
    constexpr float COL_NAME = 75.0f;
    constexpr float COL_MAP = 330.0f;
    constexpr float COL_PLAYERS = 470.0f;

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

    // Basit kilit simgesi: govde (dikdortgen) + halka (yari-daire yerine
    // ust kismi acik bir dikdortgen kenarligi).
    void DrawLockIcon(float x, float y, float size, float r, float g, float b, float a) {
        glColor4f(r, g, b, a);
        float bodyW = size * 0.8f;
        float bodyH = size * 0.55f;
        float bodyY = y + size * 0.4f;

        glBegin(GL_QUADS);
        glVertex2f(x, bodyY);
        glVertex2f(x + bodyW, bodyY);
        glVertex2f(x + bodyW, bodyY + bodyH);
        glVertex2f(x, bodyY + bodyH);
        glEnd();

        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        glVertex2f(x + bodyW * 0.2f, bodyY);
        glVertex2f(x + bodyW * 0.2f, y + size * 0.15f);
        glVertex2f(x + bodyW * 0.5f, y);
        glVertex2f(x + bodyW * 0.8f, y + size * 0.15f);
        glVertex2f(x + bodyW * 0.8f, bodyY);
        glEnd();
        glLineWidth(1.0f);
    }

    // Basit kalkan simgesi: besgen benzeri sekil.
    void DrawShieldIcon(float x, float y, float size, float r, float g, float b, float a) {
        glColor4f(r, g, b, a);
        float cx = x + size * 0.5f;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, y);
        glVertex2f(x, y + size * 0.25f);
        glVertex2f(x + size * 0.15f, y + size * 0.8f);
        glVertex2f(cx, y + size);
        glVertex2f(x + size * 0.85f, y + size * 0.8f);
        glVertex2f(x + size, y + size * 0.25f);
        glEnd();
    }

    void BuildPlaceholderRows() {
        if (!s_rowsByTab[(int)Tab::Internet].empty()) return;

        for (int i = 0; i < 14; i++) {
            ServerRow row;
            row.name = "INTERNET SERVER " + std::to_string(i + 1);
            row.mapName = (i % 2 == 0) ? "FIRSTMAP" : "DUSKLAND";
            row.ipAddress = "127.0.0.1"; // yer tutucu -- gercek kesif eklenince gercek adresle degisecek
            row.currentPlayers = (i * 3) % 16;
            row.maxPlayers = 16;
            row.hasPassword = (i % 4 == 0);
            row.hasAnticheat = (i % 3 != 0);
            s_rowsByTab[(int)Tab::Internet].push_back(row);
        }

        for (int i = 0; i < 3; i++) {
            ServerRow row;
            row.name = "LAN SERVER " + std::to_string(i + 1);
            row.mapName = "FIRSTMAP";
            row.ipAddress = "127.0.0.1";
            row.currentPlayers = i;
            row.maxPlayers = 8;
            row.hasPassword = false;
            row.hasAnticheat = true;
            s_rowsByTab[(int)Tab::Lan].push_back(row);
        }

        // History ve Favorite bilerek bos birakiliyor -- gercek bagliliklar
        // ve kaydedilen favoriler eklendiginde bu listeler doldurulacak.
    }
}

void ServerMenu::Init() {
    BuildPlaceholderRows();
}

void ServerMenu::Open() {
    BuildPlaceholderRows();
    s_selectedRow = -1;
    s_scrollOffset = 0;
    s_open = true;
}

void ServerMenu::Close() {
    s_open = false;
}

bool ServerMenu::IsOpen() {
    return s_open;
}

void ServerMenu::HandleMouseMove(int mx, int my) {
    (void)mx; (void)my; // hover efekti Render icinde anlik hesaplaniyor, burada state tutmaya gerek yok
}

void ServerMenu::HandleMouseWheel(int delta) {
    if (!s_open) return;

    auto& rows = s_rowsByTab[(int)s_activeTab];
    int visibleRows = 1; // Render'daki listH hesabina paralel, asagida daha dogru sinirlaniyor
    (void)visibleRows;

    s_scrollOffset -= delta * SCROLL_STEP;
    if (s_scrollOffset < 0) s_scrollOffset = 0;

    int maxOffset = static_cast<int>(rows.size()) - 1;
    if (maxOffset < 0) maxOffset = 0;
    if (s_scrollOffset > maxOffset) s_scrollOffset = maxOffset;
}

void ServerMenu::HandleMouseClick(int mx, int my) {
    if (!s_open) return;

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
        float tabW = g_HudFont.MeasureTextWidth(tabLabels[i], 20.0f) + 30.0f;
        if (PointIn(mx, my, tabX, contentY, tabW, TAB_ROW_H)) {
            if (s_activeTab != static_cast<Tab>(i)) {
                s_activeTab = static_cast<Tab>(i);
                s_selectedRow = -1;
                s_scrollOffset = 0;
            }
            return;
        }
        tabX += tabW + 6.0f;
    }

    float listY = contentY + TAB_ROW_H + HEADER_ROW_H + 10.0f;
    float listW = innerRight - contentX - SIDE_PANEL_W - 15.0f;
    float listH = (panelY + PANEL_H - UIWindow::GetMargin() - 10.0f) - listY;

    auto& rows = s_rowsByTab[(int)s_activeTab];
    int maxVisible = static_cast<int>(listH / ROW_H);

    for (int i = 0; i < maxVisible; i++) {
        int rowIndex = s_scrollOffset + i;
        if (rowIndex >= static_cast<int>(rows.size())) break;

        float rowY = listY + static_cast<float>(i) * ROW_H;
        if (PointIn(mx, my, contentX, rowY, listW, ROW_H)) {
            s_selectedRow = rowIndex;
            return;
        }
    }

    // Join server butonu
    float sideX = innerRight - SIDE_PANEL_W;
    float joinY = panelY + PANEL_H - UIWindow::GetMargin() - JOIN_BTN_H - 10.0f;
    if (PointIn(mx, my, sideX, joinY, SIDE_PANEL_W, JOIN_BTN_H)) {
        if (s_selectedRow >= 0 && s_selectedRow < static_cast<int>(rows.size())) {
            const ServerRow& row = rows[s_selectedRow];
            Console::Log("Baglaniliyor: " + row.name + " (" + row.ipAddress + ") -- gercek adres cozumlemesi henuz yok, test adresi kullaniliyor");
            NetClient::Connect(row.ipAddress);
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
        float tabW = g_HudFont.MeasureTextWidth(tabLabels[i], 20.0f) + 30.0f;

        FilledRect(tabX, contentY, tabW, TAB_ROW_H, active ? 0.18f : 0.06f, active ? 0.05f : 0.06f, active ? 0.05f : 0.06f, 1.0f);
        RectOutline(tabX, contentY, tabW, TAB_ROW_H, 0.5f, 0.5f, 0.5f, 0.6f);

        float labelR = active ? 1.0f : 0.75f;
        g_HudFont.DrawText(tabX + 15.0f, contentY + 8.0f, tabLabels[i], 20.0f, labelR, labelR * 0.9f, labelR * 0.9f);

        tabX += tabW + 6.0f;
    }

    // --- Sutun basliklari (sekmelerin altindaki bar) ---
    float headerY = contentY + TAB_ROW_H + 4.0f;
    float listW = innerRight - contentX - SIDE_PANEL_W - 15.0f;

    FilledRect(contentX, headerY, listW, HEADER_ROW_H, 0.1f, 0.1f, 0.1f, 0.9f);
    g_HudFont.DrawText(contentX + COL_ANTICHEAT, headerY + 4.0f, "AC", 14.0f, 0.7f, 0.7f, 0.7f);
    g_HudFont.DrawText(contentX + COL_NAME, headerY + 4.0f, "SERVER NAME", 14.0f, 0.7f, 0.7f, 0.7f);
    g_HudFont.DrawText(contentX + COL_MAP, headerY + 4.0f, "MAP", 14.0f, 0.7f, 0.7f, 0.7f);
    g_HudFont.DrawText(contentX + COL_PLAYERS, headerY + 4.0f, "PLAYERS", 14.0f, 0.7f, 0.7f, 0.7f);

    // --- Sunucu listesi ---
    float listY = headerY + HEADER_ROW_H + 6.0f;
    float listH = (panelY + PANEL_H - UIWindow::GetMargin() - 10.0f) - listY;

    RectOutline(contentX, listY, listW, listH, 0.4f, 0.4f, 0.4f, 0.6f);

    auto& rows = s_rowsByTab[(int)s_activeTab];
    int maxVisible = static_cast<int>(listH / ROW_H);

    if (rows.empty()) {
        g_HudFont.DrawText(contentX + 15.0f, listY + 15.0f, "NO SERVERS FOUND", 18.0f, 0.5f, 0.5f, 0.5f);
    }

    for (int i = 0; i < maxVisible; i++) {
        int rowIndex = s_scrollOffset + i;
        if (rowIndex >= static_cast<int>(rows.size())) break;

        float rowY = listY + static_cast<float>(i) * ROW_H;
        const ServerRow& row = rows[rowIndex];

        bool selected = (s_selectedRow == rowIndex);
        if (selected) {
            FilledRect(contentX, rowY, listW, ROW_H, 0.3f, 0.08f, 0.06f, 0.8f);
        }

        float iconY = rowY + ROW_H * 0.5f - 8.0f;

        if (row.hasPassword) {
            DrawLockIcon(contentX + COL_LOCK, iconY, 16.0f, 0.85f, 0.7f, 0.2f, 1.0f);
        }

        DrawShieldIcon(contentX + COL_ANTICHEAT, iconY, 16.0f,
            row.hasAnticheat ? 0.3f : 0.5f, row.hasAnticheat ? 0.85f : 0.3f, row.hasAnticheat ? 0.3f : 0.3f, 1.0f);

        g_HudFont.DrawText(contentX + COL_NAME, rowY + 4.0f, row.name, 15.0f, 0.85f, 0.85f, 0.85f);
        g_HudFont.DrawText(contentX + COL_MAP, rowY + 4.0f, row.mapName, 15.0f, 0.7f, 0.7f, 0.7f);

        std::string playersText = std::to_string(row.currentPlayers) + "/" + std::to_string(row.maxPlayers);
        g_HudFont.DrawText(contentX + COL_PLAYERS, rowY + 4.0f, playersText, 15.0f, 0.7f, 0.7f, 0.7f);
    }

    // Kaydirma cubugu gostergesi (basit, dekoratif -- surukleme henuz yok)
    if (static_cast<int>(rows.size()) > maxVisible) {
        float scrollBarX = contentX + listW - 6.0f;
        float scrollRatio = static_cast<float>(s_scrollOffset) / static_cast<float>(rows.size() - maxVisible);
        float thumbH = listH * (static_cast<float>(maxVisible) / static_cast<float>(rows.size()));
        if (thumbH < 20.0f) thumbH = 20.0f;
        float thumbY = listY + scrollRatio * (listH - thumbH);

        FilledRect(scrollBarX, listY, 4.0f, listH, 0.2f, 0.2f, 0.2f, 0.6f);
        FilledRect(scrollBarX, thumbY, 4.0f, thumbH, 0.6f, 0.15f, 0.1f, 0.9f);
    }

    // --- Sag panel: detay ---
    float sideX = innerRight - SIDE_PANEL_W;
    float detailH = listH - JOIN_BTN_H - 15.0f;

    RectOutline(sideX, listY, SIDE_PANEL_W, detailH, 0.4f, 0.4f, 0.4f, 0.6f);
    g_HudFont.DrawText(sideX + 10.0f, listY + 10.0f, "DETAILS", 18.0f, 0.85f, 0.85f, 0.85f);

    if (s_selectedRow >= 0 && s_selectedRow < static_cast<int>(rows.size())) {
        const ServerRow& row = rows[s_selectedRow];
        g_HudFont.DrawText(sideX + 10.0f, listY + 45.0f, row.name, 15.0f, 0.8f, 0.8f, 0.8f);
        g_HudFont.DrawText(sideX + 10.0f, listY + 70.0f, "MAP: " + row.mapName, 14.0f, 0.7f, 0.7f, 0.7f);
        g_HudFont.DrawText(sideX + 10.0f, listY + 90.0f, row.ipAddress, 13.0f, 0.55f, 0.55f, 0.55f);
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