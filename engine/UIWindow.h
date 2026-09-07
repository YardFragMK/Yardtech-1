#pragma once
#include <string>

// GoldSrc'nin klasik pencere kromunu (ikon + baslik + kapatma kutusu, iki
// katmanli cerceve: kenarliksiz siyah dis kutu + beyaz kenarlikli ic kutu),
// kirmizi-siyah-beyaz palette uyarlar. Icerigi cizmez, sadece cerceveyi cizip
// icerigin baslayabilecegi konumu geri verir. Settings, ServerMenu gibi tum
// acilir panel turleri bu sinifi kullanir.
class UIWindow {
public:
    // Bir kere, program baslarken cagrilir. Basarisiz olursa ikon cizilmez,
    // pencereler ikon olmadan calismaya devam eder.
    static bool GBLoadIcon(const std::string& iconTgaPath);

    static void Draw(float x, float y, float w, float h, const std::string& title);

    enum class Hit { None, Close };
    static Hit HandleClick(float x, float y, float w, float h, int mx, int my);

    // Icerigin baslayabilecegi Y konumu (baslik satirinin hemen alti).
    static float ContentStartY(float y);

    // Icerigin baslayabilecegi X konumu (ic kenarlik payi).
    static float ContentStartX(float x);

    // Ic kenarlikli kutunun disina ne kadar pay birakildigini (dis kutu ile
    // ic kutu arasindaki mesafe) dondurur -- genislik/yukseklik hesaplarken
    // disaridan da kullanilabilmesi icin acik.
    static float GetMargin();

private:
    static unsigned int s_iconTexture;
};