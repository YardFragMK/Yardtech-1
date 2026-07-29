#pragma once
#include"Window.h"
#include"SDL.h"
#include"Camera.h"
#include"renderer/Renderer.h"


class Engine {
public:
	Engine() = default;
	~Engine();
	bool initSystems();
	void gameLoop();
	Renderer renderer;

private:
	int windowWidth = 1280;
	int windowHeight = 720;
	SDL_GLContext glContext = nullptr;
	bool running=true;
	Uint64 lastCounter = 0;
	Window window1;
};