#pragma once
#include<SDL.h>
#include"Logger.h" 

class Window {
public:
	bool windowInit();
	SDL_Window* getWindow() const;

private:
	int windowWidth = 800;
	int windowHeight = 400;
	SDL_Window* window = nullptr;
};