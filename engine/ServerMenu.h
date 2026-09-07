#pragma once

// Sunucu listeleme paneli: Internet/Lan/History/Favorite sekmeleri, sunucu
// satirlarindan olusan liste, sag tarafta detay paneli ve "Join server"
// butonu. Bu asamada gercek bir sunucu kesfi (LAN broadcast, master server
// sorgusu) yapilmiyor -- liste ornek/placeholder satirlarla dolduruluyor,
// gorunum ve etkilesim iskeleti kurulmus oluyor. Gercek kesif eklendiginde
// bu satirlar dinamik hale getirilecektir.
class ServerMenu {
public:
    static void Init();
    static void Open();
    static void Close();
    static bool IsOpen();

    static void HandleMouseMove(int mx, int my);
    static void HandleMouseClick(int mx, int my);

    static void Render(int windowWidth, int windowHeight);
};