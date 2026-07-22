#include "KeyInput.h"
#include "Camera.h"
#include "console/Console.h"
#include<iostream>

void KeyInput::Update(bool& running, Camera& camera, float deltaTime)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)){
        Console::HandleEvent(event);

        if (!Console::IsOpen()) {
            if (event.type == SDL_MOUSEMOTION) {
                camera.ProcessMouseMovement((float)event.motion.xrel, (float)event.motion.yrel);
            }
        }

        if (event.type == SDL_QUIT){
            running = false;
        }

        if (event.type == SDL_KEYDOWN){
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
                running = false;
            }

            if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE){
                Console::Toggle();
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_N) {
                //noclip == !noclip;
            }
        }
    }


    // Klavye durumunu her frame kontrol et
    const Uint8* keys = SDL_GetKeyboardState(nullptr);


    if (keys[SDL_SCANCODE_W]){
        if (Console::IsOpen())
            return;
        camera.MoveForward(deltaTime);
    }

    if (keys[SDL_SCANCODE_S]){
        if (Console::IsOpen())
            return;
        camera.MoveBackward(deltaTime);
    }

    if (keys[SDL_SCANCODE_A]){
        if (Console::IsOpen())
            return;
        camera.MoveLeft(deltaTime);
    }

    if (keys[SDL_SCANCODE_D]){
        if (Console::IsOpen())
            return;
        camera.MoveRight(deltaTime);
    }
}