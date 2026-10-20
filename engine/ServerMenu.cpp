#include "ServerMenu.h"
#include "yg_ygui.h"
#include "NetClient.h"
#include "console/Console.h"
#include <string>
#include <vector>

using YGUI::YGUI_ICON_LOCK;
using YGUI::YGUI_ICON_SHIELD;

namespace {
    bool s_open = false;

    enum class Tab { Internet = 0, Lan, History, Favorite, Count };
    Tab s_activeTab = Tab::Internet;

    struct ServerRow {
        std::string name;
        std::string mapName;
        std::string ipAddress; // yalnizca baglanmak icin kullanilir, arayuzde gosterilmez
        int currentPlayers = 0;
        int maxPlayers = 16;
        bool hasPassword = false;
        bool hasAnticheat = true;
    };

    // Her sekmenin kendi listesi. History ve Favorite gercek bagliliklar ve
    // kaydedilen favoriler eklenene kadar bos birakilir.
    std::vector<ServerRow> s_rowsByTab[static_cast<int>(Tab::Count)];

    int s_selectedRow = -1;

    constexpr float WINDOW_WIDTH = 900.0f;
    constexpr float LIST_WIDTH = 640.0f;
    constexpr float LIST_HEIGHT = 340.0f;
    constexpr float DETAIL_WIDTH = 220.0f;
    constexpr float TAB_WIDTH = 120.0f;
    constexpr float JOIN_WIDTH = 220.0f;
    constexpr float JOIN_HEIGHT = 36.0f;
    constexpr float ICON_SIZE = 14.0f;

    // Sutun ofsetleri: liste paneli icindeki satirlar icin panelin sol kenarina gore.
    constexpr float COL_LOCK = 6.0f;
    constexpr float COL_SHIELD = 32.0f;
    constexpr float COL_NAME = 62.0f;
    constexpr float COL_MAP = 330.0f;
    constexpr float COL_PLAYERS = 470.0f;

    // Baslik satiri panelin disinda, pencerenin icinde yer alir; panelin sol
    // kenari pencere kenarindan pencere dolgusu (8) + panel ic payi (4) kadar iceridedir.
    constexpr float HEADER_OFFSET = 12.0f;

    void BuildPlaceholderRows() {
        if (!s_rowsByTab[static_cast<int>(Tab::Internet)].empty()) return;

        for (int i = 0; i < 14; i++) {
            ServerRow row;
            row.name = "INTERNET SERVER " + std::to_string(i + 1);
            row.mapName = (i % 2 == 0) ? "FIRSTMAP" : "DUSKLAND";
            row.ipAddress = "127.0.0.1"; // yer tutucu: gercek kesif eklenince degisecek
            row.currentPlayers = (i * 3) % 16;
            row.maxPlayers = 16;
            row.hasPassword = (i % 4 == 0);
            row.hasAnticheat = (i % 3 != 0);
            s_rowsByTab[static_cast<int>(Tab::Internet)].push_back(row);
        }

        for (int i = 0; i < 3; i++) {
            ServerRow row;
            row.name = "LAN SERVER " + std::to_string(i + 1);
            row.mapName = "FIRSTMAP";
            row.ipAddress = "127.0.0.1";
            row.currentPlayers = i;
            row.maxPlayers = 8;
            s_rowsByTab[static_cast<int>(Tab::Lan)].push_back(row);
        }
    }
}

void ServerMenu::Init() {
    BuildPlaceholderRows();
}

void ServerMenu::Open() {
    BuildPlaceholderRows();
    s_selectedRow = -1;
    s_open = true;
}

void ServerMenu::Close() {
    s_open = false;
}

bool ServerMenu::IsOpen() {
    return s_open;
}

void ServerMenu::Render(int windowWidth, int windowHeight) {
    if (!s_open) return;

    YGUI::SetNextWindowPos(windowWidth * 0.5f, windowHeight * 0.5f, 0.5f, 0.5f);
    YGUI::SetNextWindowSize(WINDOW_WIDTH, 0.0f); // yukseklik icerige gore
    YGUI::DrawWindow("SERVERS", &s_open);

    // --- Sekmeler ---
    static const char* tabNames[static_cast<int>(Tab::Count)] = { "INTERNET", "LAN", "HISTORY", "FAVORITE" };
    for (int i = 0; i < static_cast<int>(Tab::Count); i++) {
        if (i > 0) YGUI::SameLine();
        const bool active = (s_activeTab == static_cast<Tab>(i));
        if (YGUI::DrawSelectable(tabNames[i], active, TAB_WIDTH, 0.0f) && !active) {
            s_activeTab = static_cast<Tab>(i);
            s_selectedRow = -1;
        }
    }
    YGUI::Separator();

    // --- Sutun basliklari ---
    YGUI::DrawLabel(" "); // satiri baslatir; sutun ofsetleri bunun uzerinden kurulur
    YGUI::SameLineAt(HEADER_OFFSET + COL_SHIELD);
    YGUI::DrawLabel("AC");
    YGUI::SameLineAt(HEADER_OFFSET + COL_NAME);
    YGUI::DrawLabel("SERVER NAME");
    YGUI::SameLineAt(HEADER_OFFSET + COL_MAP);
    YGUI::DrawLabel("MAP");
    YGUI::SameLineAt(HEADER_OFFSET + COL_PLAYERS);
    YGUI::DrawLabel("PLAYERS");

    // --- Sunucu listesi ---
    const std::vector<ServerRow>& rows = s_rowsByTab[static_cast<int>(s_activeTab)];

    YGUI::BeginContentPanel("serverlist", LIST_WIDTH, LIST_HEIGHT);
    if (rows.empty()) {
        YGUI::DrawLabel("NO SERVERS FOUND");
    }
    for (int i = 0; i < static_cast<int>(rows.size()); i++) {
        const ServerRow& row = rows[i];

        const std::string rowId = "##server" + std::to_string(i);
        if (YGUI::DrawSelectable(rowId.c_str(), s_selectedRow == i)) {
            s_selectedRow = i;
        }

        // Satirin ustune, ayni yukseklikte sutunlar yerlestirilir.
        YGUI::SameLineAt(COL_LOCK);
        YGUI::DrawIcon(YGUI_ICON_LOCK, ICON_SIZE, 0.85f, 0.7f, 0.2f, row.hasPassword ? 1.0f : 0.0f);

        YGUI::SameLineAt(COL_SHIELD);
        YGUI::DrawIcon(YGUI_ICON_SHIELD, ICON_SIZE,
            row.hasAnticheat ? 0.3f : 0.6f, row.hasAnticheat ? 0.85f : 0.25f, 0.3f, 1.0f);

        YGUI::SameLineAt(COL_NAME);
        YGUI::DrawLabel("%s", row.name.c_str());
        YGUI::SameLineAt(COL_MAP);
        YGUI::DrawLabel("%s", row.mapName.c_str());
        YGUI::SameLineAt(COL_PLAYERS);
        YGUI::DrawLabel("%d/%d", row.currentPlayers, row.maxPlayers);
    }
    YGUI::EndContentPanel();

    // --- Detay paneli (IP adresi bilerek gosterilmez) ---
    YGUI::SameLine();
    YGUI::BeginContentPanel("serverdetails", DETAIL_WIDTH, LIST_HEIGHT);
    YGUI::DrawLabel("DETAILS");
    YGUI::Separator();
    if (s_selectedRow >= 0 && s_selectedRow < static_cast<int>(rows.size())) {
        const ServerRow& row = rows[s_selectedRow];
        YGUI::DrawLabel("%s", row.name.c_str());
        YGUI::Spacing();
        YGUI::DrawLabel("MAP: %s", row.mapName.c_str());
        YGUI::DrawLabel("PLAYERS: %d/%d", row.currentPlayers, row.maxPlayers);
        YGUI::DrawLabel("PASSWORD: %s", row.hasPassword ? "YES" : "NO");
        YGUI::DrawLabel("ANTI-CHEAT: %s", row.hasAnticheat ? "ON" : "OFF");
    }
    else {
        YGUI::DrawLabel("SELECT A SERVER");
    }
    YGUI::EndContentPanel();

    // --- Alt cubuk: sunucu sayisi (solda) ve katil butonu (sagda) ---
    YGUI::DrawLabel("%d SERVERS", static_cast<int>(rows.size()));
    YGUI::SameLineAt(WINDOW_WIDTH - JOIN_WIDTH - 12.0f);
    if (YGUI::DrawButton("JOIN SERVER", JOIN_WIDTH, JOIN_HEIGHT)) {
        if (s_selectedRow >= 0 && s_selectedRow < static_cast<int>(rows.size())) {
            const ServerRow& row = rows[s_selectedRow];
            Console::Log("Baglaniliyor: " + row.name);
            NetClient::Connect(row.ipAddress);
            s_open = false;
        }
    }

    YGUI::EndWindow();
}