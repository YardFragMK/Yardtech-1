#include"KeyInput.h"
#include"console/Console.h"


void KeyInput::Update(bool& running) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
				Console::Toggle();
			}
			break;
		}
		

		
	}
}