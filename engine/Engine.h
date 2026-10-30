#pragma once
#include"Windows.h"
#include"Window.h"
#include"SDL.h"
#include"renderer/Renderer.h"
#include"BSPMap.h"
#include"VFSypak.h"

class Engine {
public:
	Engine() = default;
	~Engine();
	bool initSystems(int argc = 0, char* argv[] = nullptr);
	void gameLoop();
	void windowsError(const std::wstring& message, const std::wstring& title);

private:
	int windowWidth = 1920;
	int windowHeight = 1080;
	SDL_GLContext glContext = nullptr;
	bool running=true;
	Uint64 lastCounter = 0;
	Window window1;
	void RenderFrame(float alpha, float deltaTime);

	std::string m_overrideMap = "";

};

extern Renderer renderer;
extern VirtualFileSystem vfs;