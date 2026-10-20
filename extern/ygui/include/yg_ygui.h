#pragma once

#ifdef YGUI_DLL_EXPORTS
#define YGUI_API __declspec(dllexport)
#else
#define YGUI_API __declspec(dllimport)
#endif

namespace YGUI {
    YGUI_API void Initialize();
    YGUI_API struct ImGuiContext* GetimguiContext();
    YGUI_API void NewFrame(float deltaTime, float displayWidth, float displayHeight);
    YGUI_API void EndFrame();
    YGUI_API void Shutdown();

    // ygui elements
    YGUI_API void DrawWindow(const char* title, bool* open);
    YGUI_API void EndWindow();
    YGUI_API bool DrawButton(const char* label, float width = 100.0f, float height = 30.0f);
    YGUI_API void DrawLabel(const char* fmt, ...);
    YGUI_API bool DrawTextBox(const char* label, char* buffer, size_t bufferSize);
    YGUI_API bool DrawCheckBox(const char* label, bool* value);
    YGUI_API bool DrawSliderFloat(const char* label, float* value, float min, float max);
    YGUI_API void BeginContentPanel(const char* str_id, float width = 0.0f, float height = 0.0f);
    YGUI_API void EndContentPanel();

    YGUI_API void DrawServerBrowser(bool* open);

    YGUI_API void SameLine();
    YGUI_API void Separator();
    YGUI_API void Spacing();

    YGUI_API void AddMousePosEvent(float x, float y);
    YGUI_API void AddMouseButtonEvent(int button, bool down);
    YGUI_API void AddMouseWheelEvent(float wheelX, float wheelY);
    YGUI_API void AddKeyEvent(int keycode, bool down);
    YGUI_API void AddInputCharacter(unsigned int codepoint);

    // Ekran-uzayi cizim ilkelleri (HUD gibi pencere gerektirmeyen katmanlar icin).
    // Koordinatlar piksel, orijin sol-ust. Cizimler tum pencerelerin ARKASINDA kalir.
    YGUI_API void ScreenFillRect(float x, float y, float w, float h, float r, float g, float b, float a);
    YGUI_API void ScreenRect(float x, float y, float w, float h, float r, float g, float b, float a, float thickness = 1.0f);
    YGUI_API void ScreenLine(float x0, float y0, float x1, float y1, float r, float g, float b, float a, float thickness = 1.0f);
    // xyPairs: x0,y0,x1,y1,... ; noktalar saat yonunde ve konveks olmali (en fazla 16 nokta).
    YGUI_API void ScreenConvexPoly(const float* xyPairs, int pointCount, float r, float g, float b, float a);
    // Cizilen metnin genisligini dondurur. size: font boyutu (piksel).
    YGUI_API float ScreenText(float x, float y, const char* text, float size, float r, float g, float b, float a);
    YGUI_API float ScreenTextWidth(const char* text, float size);

    // --- Menu yerlesimi ve ek kontroller ---
    // Sonraki pencerenin konumu. pivot (0..1): (0.5, 0.5) pencerenin merkezini verilen noktaya koyar.
    YGUI_API void SetNextWindowPos(float x, float y, float pivotX = 0.0f, float pivotY = 0.0f);
    // Sonraki pencerenin boyutu. Eksenlerden biri 0 ise o eksen icerige gore otomatik boyutlanir.
    YGUI_API void SetNextWindowSize(float width, float height);
    // Liste satiri ve sekme icin secilebilir oge. Tiklaninca true doner; width/height 0 ise otomatik.
    YGUI_API bool DrawSelectable(const char* label, bool selected, float width = 0.0f, float height = 0.0f);
    // Ayni satirda, pencerenin sol kenarindan offsetX pikselde devam eder (sutun hizalama).
    YGUI_API void SameLineAt(float offsetX);

    constexpr int YGUI_ICON_LOCK = 0;
    constexpr int YGUI_ICON_SHIELD = 1;
    // size x size yer kaplayan basit bir simge cizer; a <= 0 ise yalnizca yer ayirir.
    YGUI_API void DrawIcon(int iconId, float size, float r, float g, float b, float a);
}
