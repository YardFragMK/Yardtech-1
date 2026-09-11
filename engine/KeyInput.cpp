#include "KeyInput.h"
#include "Camera.h"
#include "console/Console.h"
#include "../game/src/Player.h"
#include <iostream>
#include "console/CVar.h"
#include "Engine.h"
#include "GameState.h"
#include "MainMenu.h"
#include "PauseMenu.h"
#include "Settings.h"
#include "ServerMenu.h"

void KeyInput::Update(bool& running, Camera& camera, float deltaTime)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        Console::HandleEvent(event);

        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEMOTION) {
                if (ServerMenu::IsOpen()) {
                    ServerMenu::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else if (Settings::IsOpen()) {
                    Settings::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else if (g_State == GameState::Playing) {
                    camera.ProcessMouseMovement((float)event.motion.xrel, (float)event.motion.yrel);
                }
                else if (g_State == GameState::Paused) {
                    PauseMenu::HandleMouseMove(event.motion.x, event.motion.y);
                }
                else {
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

            if (g_State == GameState::Playing && !Console::IsOpen() && !Settings::IsOpen() && !ServerMenu::IsOpen()) {
                if (event.key.keysym.scancode == SDL_SCANCODE_1) {
                    g_Player.SwitchWeapon(PISTOL);
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_2) {
                    g_Player.SwitchWeapon(SHOTGUN);
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_R && !event.key.repeat) {
                    g_Player.ReloadCurrentWeapon();
                }
            }
        }

        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (ServerMenu::IsOpen()) {
                        ServerMenu::HandleMouseClick(event.button.x, event.button.y);
                    }
                    else if (Settings::IsOpen()) {
                        Settings::HandleMouseDown(event.button.x, event.button.y);
                    }
                    else if (g_State == GameState::Playing) {
                        g_Player.attack(1);
                    }
                    else if (g_State == GameState::Paused) {
                        PauseMenu::HandleMouseClick(event.button.x, event.button.y);
                    }
                    else {
                        MainMenu::HandleMouseClick(event.button.x, event.button.y);
                    }
                }

                if (event.button.button == SDL_BUTTON_RIGHT && g_State == GameState::Playing && !Settings::IsOpen() && !ServerMenu::IsOpen()) {
                    g_Player.attack(2);
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    Settings::HandleMouseUp();
                }
            }
            if (event.type == SDL_MOUSEWHEEL && !Console::IsOpen()) {
                if (ServerMenu::IsOpen()) {
                    ServerMenu::HandleMouseWheel(event.wheel.y);
                }
            }
        }
    }

    if (Console::IsOpen()) return;
    if (g_State != GameState::Playing) return;
}