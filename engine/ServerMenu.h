#pragma once

// Sunucu listeleme penceresi: YGUI ile cizilir. Internet/Lan/History/Favorite
// sekmeleri, her sekmenin kendi (su an placeholder) sunucu listesi, kilit ve
// anti-hile simgeleri, harita adi ve oyuncu sayisi sutunlari. Kaydirmayi ImGui
// yonetir. Gercek sunucu kesfi henuz yok; satirlar ornek veriyle doldurulur.
class ServerMenu {
public:
    static void Init();
    static void Open();
    static void Close();
    static bool IsOpen();

    // YGUI::NewFrame ile YGUI::EndFrame arasinda cagrilmalidir.
    static void Render(int windowWidth, int windowHeight);
};