#include "KeyInput.h"
#include "console/Console.h"
#include "GameState.h"
#include "MainMenu.h"
#include "PauseMenu.h"
#include "Settings.h"
#include "ServerMenu.h"
#include "InputBridge.h"
#include <SDL.h>

void KeyInput::Update(bool& running)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        Console::HandleEvent(event);

        // Oyun girdisi (bakis, hareket, atis, silah tuslari) client.dll'e gider;
        // client, girdinin oyuna ait olup olmadigina kendisi karar verir.
        ForwardEventToClient(event, Console::IsOpen());

        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEMOTION) {
                if (ServerMenu::IsOpen()) {
                    ServerMenu::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else if (Settings::IsOpen()) {
                    Settings::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else if (g_State == GameState::Paused) {
                    PauseMenu::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else if (g_State != GameState::Playing) {
                    MainMenu::HandleMouseMove(event.motion.x, event.motion.y);
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

        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (ServerMenu::IsOpen()) {
                    ServerMenu::HandleMouseClick(event.button.x, event.button.y);
                }
                else if (Settings::IsOpen()) {
                    Settings::HandleMouseDown(event.button.x, event.button.y);
                }
                else if (g_State == GameState::Paused) {
                    PauseMenu::HandleMouseClick(event.button.x, event.button.y);
                }
                else if (g_State != GameState::Playing) {
                    MainMenu::HandleMouseClick(event.button.x, event.button.y);
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                Settings::HandleMouseUp();
            }

            if (event.type == SDL_MOUSEWHEEL && ServerMenu::IsOpen()) {
                ServerMenu::HandleMouseWheel(event.wheel.y);
            }
        }
    }
}