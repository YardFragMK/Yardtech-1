#pragma once
#include"Window.h"
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
	SDL_GLContext glContext = nullptr;
	bool running=true;
	Uint64 lastCounter = 0;
	Window window1;

};