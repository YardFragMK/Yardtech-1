#pragma once
#include"Window.h"
#include"SDL.h"
#include"Camera.h"
#include"renderer/Renderer.h"
#include"BSPMap.h"

class Engine {
public:
	Engine() = default;
	~Engine();
	bool initSystems();
	void gameLoop();

private:
	int windowWidth = 1920;
	int windowHeight = 1080;
	SDL_GLContext glContext = nullptr;
	bool running=true;
	Uint64 lastCounter = 0;
	Window window1;
	void RenderFrame();

};

extern 	Renderer renderer;