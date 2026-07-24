#pragma once
#include"Window.h"
#include"SDL.h"
#include"Camera.h"
#include"renderer/Renderer.h"
#include"map/BVH.h"
#include"map/DynamicLightManager.h"

class Engine {
public:
	Engine() = default;
	~Engine();
	bool initSystems();
	void gameLoop();
	Renderer renderer;
	BVH worldBVH;
	DynamicLightManager dynamicLights;

private:
	int windowWidth = 1280;
	int windowHeight = 720;
	SDL_GLContext glContext = nullptr;
	bool running=true;
	Uint64 lastCounter = 0;
	Window window1;

};