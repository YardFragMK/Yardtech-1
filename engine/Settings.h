#pragma once

// Ayarlar penceresi: YGUI (ImGui tabanli) ile cizilir. Sekmeler ve kontroller
// her karede YGUI cagrilariyla tanimlanir; fare ve klavye girdisini ImGui isler,
// bu sinif yalnizca acik/kapali durumunu tutar.
class Settings {
public:
    static void Open();
    static void Close();
    static bool IsOpen();

    // YGUI::NewFrame ile YGUI::EndFrame arasinda cagrilmalidir.
    static void Render(int windowWidth, int windowHeight);

private:
    enum class Category { Graphics = 0, Sound, Gameplay, Count };

    static bool s_open;
    static Category s_activeCategory;

    static void DrawGraphicsTab();
    static void DrawSoundTab();
    static void DrawGameplayTab();
};