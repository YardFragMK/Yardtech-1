#include"KeyInput.h"
#include"Camera.h"
#include"console/Console.h"

void KeyInput::Update(bool& running, Camera& camera, float deltaTime) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false; 
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_ESCAPE:
				running = false;
				break;
			case SDL_SCANCODE_GRAVE:
				Console::Toggle(); break;

			case SDL_SCANCODE_W:
				camera.MoveForward(deltaTime); break;
			
			case SDL_SCANCODE_A:
				camera.MoveLeft(deltaTime); break;

			case SDL_SCANCODE_S:
				camera.MoveBackward(deltaTime); break;

			case SDL_SCANCODE_D:
				camera.MoveRight(deltaTime); break;
			case SDL_MOUSEMOTION:
				camera.ProcessMouseMovement(static_cast<float>(event.motion.xrel),
											static_cast<float>(event.motion.yrel)); break;
			}


			break;
		}
	
	}
}


