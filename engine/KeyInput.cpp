#include "KeyInput.h"
#include "console/Console.h"
#include "GameState.h"
#include "MainMenu.h"
#include "PauseMenu.h"
#include "Settings.h"
#include "ServerMenu.h"
#include "InputBridge.h"
#include "YguiBridge.h"
#include <SDL.h>

void KeyInput::Update(bool& running)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        Console::HandleEvent(event);

        // Oyun girdisi client.dll'e, menu girdisi YGUI'ye gider. YGUI penceresi
        // yokken olaylar ImGui tarafinda zararsizca yok sayilir.
        ForwardEventToClient(event, Console::IsOpen());
        ForwardEventToYgui(event, Console::IsOpen());

        // Ayarlar veya sunucu listesi acikken altindaki FFP menulere girdi gitmez.
        const bool modalOpen = Settings::IsOpen() || ServerMenu::IsOpen();

        if (!Console::IsOpen() && !modalOpen) {
            if (event.type == SDL_MOUSEMOTION) {
                if (g_State == GameState::Paused) {
                    PauseMenu::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else if (g_State != GameState::Playing) {
                    MainMenu::HandleMouseMove(event.motion.x, event.motion.y);
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (g_State == GameState::Paused) {
                    PauseMenu::HandleMouseClick(event.button.x, event.button.y);
                }
                else if (g_State != GameState::Playing) {
                    MainMenu::HandleMouseClick(event.button.x, event.button.y);
                }
            }
        }

        if (event.type == SDL_QUIT) {
            running = false;
        }

        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                if (ServerMenu::IsOpen()) {
                    ServerMenu::Close();
                }
                else if (Settings::IsOpen()) {
                    Settings::Close();
                }
                else if (g_State == GameState::Paused) {
                    ResumeFromPause();
                }
                else if (g_State == GameState::Playing) {
                    EnterPaused();
                }
                else {
                    running = false;
                }
            }

            if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
                Console::Toggle();
            }
        }
    }
}