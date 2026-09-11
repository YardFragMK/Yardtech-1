#pragma once

// Sunucu listeleme paneli: Internet/Lan/History/Favorite sekmeleri, her
// sekmenin kendi (su an placeholder) sunucu listesi, kilit/anti-hile
// ikonlari, harita adi ve oyuncu sayisi sutunlari, fare tekerlegiyle
// kaydirma. Gercek bir sunucu kesfi (LAN broadcast, master server sorgusu)
// henuz yok -- satirlar ornek veriyle doldurulmus, gorunum ve etkilesim
// iskeleti calisir durumda.
class ServerMenu {
public:
    static void Init();
    static void Open();
    static void Close();
    static bool IsOpen();

    static void HandleMouseMove(int mx, int my);
    static void HandleMouseClick(int mx, int my);
    static void HandleMouseWheel(int delta);

    static void Render(int windowWidth, int windowHeight);
};