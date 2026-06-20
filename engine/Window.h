#pragma once
#include<SDL.h>
#include"Logger.h" 
class Window {
public:
	bool windowInit();
	SDL_Window* getWindow() const;

private:
	int windowWidth = 1280;
	int windowHeight = 720;
	SDL_Window* window = nullptr;
};