#include "KeyInput.h"
#include "Camera.h"
#include "console/Console.h"
#include"../game/src/Player.h"
#include"../game/src/Player.h"
#include<iostream>

void KeyInput::Update(bool& running, Camera& camera, float deltaTime)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        Console::HandleEvent(event);

        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEMOTION) {
                camera.ProcessMouseMovement((float)event.motion.xrel, (float)event.motion.yrel);
            }
        }

        if (event.type == SDL_QUIT) {
            running = false;
        }

        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
            }

            if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
                Console::Toggle();
            }

        }
        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Sol tuşa BASILDI
                    g_Player.attack(1);
                }

                if (event.button.button == SDL_BUTTON_RIGHT) {
                    // Sağ tuşa BASILDI
                    g_Player.attack(2);
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Sol tuş BIRAKILDI
                }

                if (event.button.button == SDL_BUTTON_RIGHT) {
                    // Sağ tuş BIRAKILDI
                }
            }
        }

    }


    if (Console::IsOpen()) {
        return; // konsol acikken hareket tamamen atlanir
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    if (keys[SDL_SCANCODE_W]) camera.MoveForward(deltaTime);
    if (keys[SDL_SCANCODE_S]) camera.MoveBackward(deltaTime);
    if (keys[SDL_SCANCODE_A]) camera.MoveLeft(deltaTime);
    if (keys[SDL_SCANCODE_D]) camera.MoveRight(deltaTime);
}