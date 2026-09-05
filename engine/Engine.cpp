#include<iostream>
#include<enet/enet.h>
//#include<glad/glad.h>
#include<SDL.h>
#include<cstdlib> 
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
#include"Frustum.h"
#include"console/CVar.h"
#include"../game/src/Player.h"
#include"EntityParser.h"
#include"PlayerMovement.h"
#include"ClientPlayerController.h"
#include"Skybox.h"
#include"HUD.h"
#include"GameState.h"
#include"MainMenu.h"
#include"MapLoader.h"
#include"BitmapFont.h"
#include"Settings.h" 
#include"PauseMenu.h"


void TestEnetInit() {
	if (enet_initialize() != 0) {
		Logger::info("ENet baslatilamadi!");
		return;
	}
	Logger::info("ENet basariyla baslatildi.");
	enet_deinitialize();
}

Renderer renderer;
BSPMap g_Map;

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

	// Gercek pencere boyutunu al (DPI olcekleme/farkli cozunurluk ihtimaline karsi
	// sabit degerlere guvenmek yerine SDL'den dogrudan sor).
	int actualW = 0, actualH = 0;
	SDL_GetWindowSize(window1.getWindow(), &actualW, &actualH);
	windowWidth = actualW;
	windowHeight = actualH;
	Logger::info("Pencere boyutu: " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));


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

	//=========================================================
	// Bitmap Font
	//=========================================================
	if (!g_HudFont.Load("nvs1/gfx/hud_font.tga")) {
		Logger::error("HUD fontu yuklenemedi.");
	}

	//=========================================================
	// Main Menu
	//=========================================================
	//SDL_SetRelativeMouseMode(SDL_FALSE);
	MainMenu::Init();
	MainMenu::LoadBackgroundImage("nvs1/gfx/env/classiclandft.tga");

	const float btnX = 70.0f;
	const float btnW = 320.0f;
	const float btnH = 55.0f;
	const float btnSpacing = 68.0f;
	const float topMargin = 30.0f;

	MainMenu::AddButton(btnX, topMargin + btnSpacing * 0, btnW, btnH, "NEW GAME", []() {
		ReadEntityLump("nvs1/map/firstmap.bsp");
		LoadMap("firstmap");
		EnterPlaying();
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 1, btnW, btnH, "LOAD GAME", []() {
		Console::Log("Load game henuz baglanmadi");
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 2, btnW, btnH, "HOW TO PLAY", []() {
		Console::Log("How to play henuz baglanmadi");
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 3, btnW, btnH, "SETTINGS", []() {
		Settings::Open();
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 4, btnW, btnH, "QUIT", []() {
		SDL_Event quitEvent;
		quitEvent.type = SDL_QUIT;
		SDL_PushEvent(&quitEvent);
		});

	PauseMenu::Init();

	//=========================================================
	// Enet
	//=========================================================
	TestEnetInit();

	lastCounter = SDL_GetPerformanceCounter();
	Logger::info("Engine initalize edildi.");

	return true;

}

//=========================================================
//Game Loop
//=========================================================
void Engine::gameLoop() {
	while (running) {
		//=========================================================
		// DELTATIME
		//=========================================================
		Uint64 currentCounter = SDL_GetPerformanceCounter();
		float deltaTime =
			static_cast<float>(currentCounter - lastCounter) /
			static_cast<float>(SDL_GetPerformanceFrequency());
		lastCounter = currentCounter;

		Time::Update(deltaTime);

		glm::vec3 oldPos = g_Camera.position;

		//=========================================================
        // Input / Update
        //=========================================================
		KeyInput::Update(running, g_Camera, deltaTime);
		Console::Update(deltaTime);
		g_Camera.Update(deltaTime);
		if (g_State == GameState::Playing) {
			UpdatePlayerPhysics(deltaTime, oldPos); 
		}
		else if (g_State == GameState::MenuLive) {
			MainMenu::Update(deltaTime); 
		}
 
		RenderFrame();
	}
}


void Engine::RenderFrame() {
	renderer.BeginFrame(g_Camera);

	if (g_State == GameState::MenuLive || g_State == GameState::Playing || g_State == GameState::Paused) {
		g_Skybox.Render(g_Camera.GetEyePosition());

		Frustum frustum = Frustum::FromViewProjection(
			renderer.GetProjectionMatrix() * renderer.GetViewMatrix()
		);

		g_Map.RenderWorld(frustum);
		g_Map.RenderBrushEntities(frustum);
	}

	renderer.EndFrame();

	if (g_State == GameState::Playing) {
		HUD::Render(windowWidth, windowHeight);
	}
	else if (g_State == GameState::Paused) {
		HUD::Render(windowWidth, windowHeight);
		PauseMenu::Render(windowWidth, windowHeight);
	}
	else {
		MainMenu::Render(windowWidth, windowHeight);
	}

	Settings::Render(windowWidth, windowHeight);
	Console::Render(windowWidth, windowHeight);

	SDL_GL_SwapWindow(window1.getWindow());
}
