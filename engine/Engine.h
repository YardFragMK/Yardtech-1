#pragma once
#include"Windows.h"
#include"Window.h"
#include"SDL.h"
#include"Camera.h"
#include"RTRenderer.h"
#include"BSPMap.h"

class Engine {
public:
	Engine() = default;
	~Engine();
	bool initSystems();
	void gameLoop();
	void windowsError(const std::wstring& message, const std::wstring& title);

private:
	int windowWidth = 1920;
	int windowHeight = 1080;
	bool running = true;
	Uint64 lastCounter = 0;
	Window window1;
	void RenderFrame();

};