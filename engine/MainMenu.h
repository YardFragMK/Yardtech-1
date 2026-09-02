#pragma once
#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <GL/gl.h>

struct MenuButton {
    float x, y, w, h;
    std::string label;
    std::function<void()> onClick;
    bool hovered = false;
};

class MainMenu {
public:
    static void Init();
    static void Update(float deltaTime);
    static void Render(int windowWidth, int windowHeight);

    static void HandleMouseMove(int mx, int my);
    static void HandleMouseClick(int mx, int my);

    // Kendi buton duzenini kurmak icin: once ClearButtons(), sonra istedigin kadar AddButton.
    static void ClearButtons();
    static void AddButton(float x, float y, float w, float h, const std::string& label, std::function<void()> onClick);

    // MenuStatic arkaplan resmi (TGA). Basarisiz olursa duz koyu renge duser.
    static bool LoadBackgroundImage(const std::string& tgaPath);

private:
    static void UpdateLiveCamera(float deltaTime);

    static GLuint s_bgTexture;
    static std::vector<MenuButton> s_buttons;
    static float s_liveCamYaw;
};