#include<iostream>
#include<SDL.h>
#include"Engine.h"
#include"Logger.h"
#include"Time.h"
#include"KeyInput.h"



Engine::~Engine()
{
	if (glContext)
		SDL_GL_DeleteContext(glContext);

	if (window)
		SDL_DestroyWindow(window);
	SDL_Quit();
}
 
bool Engine::initSystems() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("SDL video system initalize edildi.");

	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);



	//---PENCERE---


	window = SDL_CreateWindow(
		"Yardtech 1",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		windowWidth,
		windowHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	if (!window) {
		Logger::error(SDL_GetError());
		 return false;
	}
	Logger::info("window olusturuldu.");

	//---OPENGL CONTEXT---
	glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("glContext olusturuldu.");
	// Context ve pencere baglantisi
	if (SDL_GL_MakeCurrent(window, glContext) != 0) {
		Logger::error(SDL_GetError());
		return false;
	} 
	Logger::info("window ve glContext birbirine baglandi.");

	lastCounter = SDL_GetPerformanceCounter();
	Logger::info("Engine initalize edildi.");
	return true;

}
void Engine::gameLoop() {
	while (running) {
		int currentCounter = SDL_GetPerformanceCounter();
		float deltaTime = (currentCounter - lastCounter)/SDL_GetPerformanceFrequency();
		lastCounter = currentCounter;
		Time::Update(deltaTime);
		KeyInput::Update(running);
		SDL_Delay(1);//update(); renderer();
	}
}
