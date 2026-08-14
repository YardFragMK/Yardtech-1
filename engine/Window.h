#pragma once
#include<SDL.h>
#include"Logger.h" 

class Window {
public:
	bool windowInit();
	SDL_Window* getWindow() const;

private:
	int windowWidth = 1920;
	int windowHeight = 1080;
	SDL_Window* window = nullptr;
};