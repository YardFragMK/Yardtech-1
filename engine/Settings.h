#pragma once
#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <GL/gl.h>

// Ayarlar ekrani: siyah panel, ust kisimda kategori sekmeleri (GRAPHICS/VIDEO/
// SOUND/GAMEPLAY), altinda ince kirmizi serit, seritin altinda secili kategorinin
// kontrolleri (slider/toggle). Herhangi bir GameState'in ustune bagimsiz bir
// overlay olarak cizilir -- ana menuden de, ileride kurulacak pause menusunden
// de ayni Open() cagrisiyla acilabilir.
class Settings {
public:
    static void Open();
    static void Close();
    static bool IsOpen();

    static void HandleMouseMove(int mx, int my);
    static void HandleMouseDown(int mx, int my);
    static void HandleMouseUp();

    static void Render(int windowWidth, int windowHeight);

private:
    struct SliderControl {
        std::string label;
        float* valuePtr;
        float minVal, maxVal;
        float dx, dy, w; // panel origin'ine gore relatif konum
        bool dragging = false;
    };

    struct ToggleControl {
        std::string label;
        std::function<bool()> getValue;
        std::function<void(bool)> setValue;
        float dx, dy;
    };

    enum class Category { Graphics = 0, Video, Sound, Gameplay, Count };

    static bool s_open;
    static bool s_built;
    static Category s_activeCategory;

    static std::vector<SliderControl> s_sliders[(int)Category::Count];
    static std::vector<ToggleControl> s_toggles[(int)Category::Count];

    // Bir onceki Render cagrisinda hesaplanan panel konumu. Input isleme bir
    // frame geriden bu degerleri kullanir -- pencere boyutu frame'ler arasi
    // degismedigi icin pratikte sorun yaratmaz.
    static float s_panelX, s_panelY, s_panelW, s_panelH;

    static void BuildControls();
    static void RenderCategoryTabs();
    static void RenderActiveCategory();
};