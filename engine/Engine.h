#pragma once
#include"SDL.h"

class Engine {
public:
	Engine() = default;
	~Engine();
	bool initSystems();
	void gameLoop();

private:
	int windowWidth = 1280;
	int windowHeight = 720;
	SDL_Window* window = nullptr;
	SDL_GLContext glContext = nullptr;
	bool running=true;
	int lastCounter = 0;

};