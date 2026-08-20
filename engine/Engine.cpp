#include<iostream>
//#include<glad/glad.h>
#include<SDL.h>
#include"Engine.h"
#include"Logger.h"
#include"Time.h"
#include"KeyInput.h"
#include"Window.h"
#include"console/Console.h"
#include"renderer/Renderer.h"
#include"Camera.h"
#include"BSPReader.h"
#include"BSPMap.h"
#include"BSPFormat.h"
#include "Frustum.h"
#include "console/CVar.h"

Renderer renderer;

Engine::~Engine(){
	if (glContext) {
		SDL_GL_DeleteContext(glContext);
	}
	if (window1.getWindow()) {
		SDL_DestroyWindow(window1.getWindow());
	}
	SDL_Quit();
}
 
//=========================================================
//Engine 
//=========================================================
bool Engine::initSystems() {

	Console::Init();
	Logger::info("Console initalize edildi");

	//=========================================================
	//Window Init
	//=========================================================
	if (!window1.windowInit()) {
		Logger::error("Window olusturulamadi");
		return false;
	}

	//=========================================================
	//Opengl Context
	//=========================================================
	glContext = SDL_GL_CreateContext(window1.getWindow());
	if (!glContext) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("glContext olusturuldu.");

	//=========================================================
	//Context and Window
	//=========================================================
	if (SDL_GL_MakeCurrent(window1.getWindow(), glContext) != 0) {
		Logger::error(SDL_GetError());
		return false;
	} 
	Logger::info("window ve glContext birbirine baglandi.");

	/*
	//=========================================================
	//GLAD
	//=========================================================
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		Logger::error("Glad yüklenemedi");
		return false;
	}
	Logger::info("glad initialize edildi.");
	*/

	//=========================================================
	//Renderer Init
	//=========================================================
	if (!renderer.Init(windowWidth, windowHeight)){
		Logger::error("Renderer initialize edilemedi.");
		return false;
	}
	Logger::info("Renderer initialize edildi.");


	lastCounter = SDL_GetPerformanceCounter();
	SDL_SetRelativeMouseMode(SDL_TRUE);
	Logger::info("Engine initalize edildi.");

	if (!g_Map.Load("nvs1/map/cs_assault.bsp", {"", "map/", "wads/", "textures/"})) {
		Logger::error("BSP yuklenemedi.");
		// return false;
	}

	return true;

}

//=========================================================
//Game Loop
//=========================================================
void Engine::gameLoop() {
	while (running) {
		//DELTATIME
		Uint64 currentCounter = SDL_GetPerformanceCounter();
		float deltaTime =
			static_cast<float>(currentCounter - lastCounter) /
			static_cast<float>(SDL_GetPerformanceFrequency());
		lastCounter = currentCounter;

		Time::Update(deltaTime);
		glm::vec3 oldPos = g_Camera.position;
		KeyInput::Update(running, g_Camera, deltaTime);
		Console::Update(deltaTime);

		g_Camera.Update(deltaTime);

		// --- collision (noclip kapaliyken) ---
		if (!g_CVar.cm_noclip) {
			glm::vec3 newPos = g_Camera.position;
			if (newPos != oldPos) {
				g_Camera.position = g_Map.SlideMove(oldPos, newPos, 1);
			}
		}

		renderer.BeginFrame(g_Camera);

		Frustum frustum = Frustum::FromViewProjection(
			renderer.GetProjectionMatrix() * renderer.GetViewMatrix()
		);
	
		g_Map.RenderWorld(frustum);
		g_Map.RenderBrushEntities(frustum);
	
		//renderer.DrawTestTriangle();
		renderer.EndFrame();
		Console::Render(windowWidth, windowHeight);

		SDL_GL_SwapWindow(window1.getWindow());


	}
}
