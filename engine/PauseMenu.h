#pragma once

class PauseMenu {
public:
    static void Init();
    static void HandleMouseMove(int mx, int my);
    static void HandleMouseClick(int mx, int my);
    static void Render(int windowWidth, int windowHeight);
};