#include<glad/glad.h>
#include<iostream>
#include<SDL.h>
#include"Engine.h"
#include"Logger.h"
#include"Time.h"
#include"KeyInput.h"
#include"Window.h"
#include"console/Console.h"
#include"renderer/Renderer.h"



Engine::~Engine()
{
	if (glContext) {
		SDL_GL_DeleteContext(glContext);
	}
	if (window1.getWindow()) {
		SDL_DestroyWindow(window1.getWindow());
	}
	SDL_Quit();
}
 
bool Engine::initSystems() {
	Console::Init();
	Logger::info("Console initalize edildi");

	if (!window1.windowInit()) {
		Logger::error("Window olusturulamadi");
		return false;
	}


	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	//OPENGL CONTEXT
	glContext = SDL_GL_CreateContext(window1.getWindow());
	if (!glContext) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("glContext olusturuldu.");

	// CONTEXT AND WINDOW
	if (SDL_GL_MakeCurrent(window1.getWindow(), glContext) != 0) {
		Logger::error(SDL_GetError());
		return false;
	} 
	Logger::info("window ve glContext birbirine baglandi.");


	//GLAD
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		Logger::error("Glad yüklenemedi");
		return false;
	}
	Logger::info("glad initialize edildi.");

	//RENDERER
	if (!Renderer::Init()){
		Logger::error("Renderer initialize edilemedi.");
		return false;
	}
	Logger::info("Renderer initialize edildi.");

	lastCounter = SDL_GetPerformanceCounter();
	Logger::info("Engine initalize edildi.");
	return true;

}
void Engine::gameLoop() {
	while (running) {
		Uint64 currentCounter = SDL_GetPerformanceCounter();
		float deltaTime =
			static_cast<float>(currentCounter - lastCounter) /
			static_cast<float>(SDL_GetPerformanceFrequency());
		lastCounter = currentCounter;
		Time::Update(deltaTime);
		KeyInput::Update(running);
		Renderer::BeginFrame();

		/*
		BSP Render
		Entity Render
		HUD Render
		Console Render
		*/

		Renderer::EndFrame();

		SDL_GL_SwapWindow(
			window1.getWindow());

		SDL_Delay(1);
	}
}
