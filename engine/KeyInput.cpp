#include"KeyInput.h"
#include<SDL.h>


void Input::Update(bool& running) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
		}
	}
}