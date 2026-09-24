#include<iostream>
#include<filesystem>
#include<enet/enet.h>
#include<SDL.h>
#include<cstdlib>
#include"Engine.h"
#include"Logger.h"
#include"Time.h"
#include"KeyInput.h"
#include"Window.h"
#include"console/Console.h"
#include"RTRenderer.h"
#include"Camera.h"
#include"BSPReader.h"
#include"BSPMap.h"
#include"BSPFormat.h"
#include"console/CVar.h"
#include"../game/src/Player.h"
#include"EntityParser.h"
#include"PlayerMovement.h"
#include"ClientPlayerController.h"
#include"HUD.h"
#include"GameState.h"
#include"MainMenu.h"
#include"MapLoader.h"
#include"BitmapFont.h"
#include"Settings.h"
#include"PauseMenu.h"
#include"NetClient.h"
#include"BSPMapRenderer.h"
#include"ServerMenu.h"
#include"UIWindow.h"
#include"../game/src/BotManager.h"

BSPMap g_Map;

Engine::~Engine() {
	NetClient::Disconnect();
	enet_deinitialize();

	g_RTRenderer.Shutdown();

	if (window1.getWindow()) {
		SDL_DestroyWindow(window1.getWindow());
	}
	SDL_Quit();
}

//=========================================================
//Engine
//=========================================================
bool Engine::initSystems() {
	std::filesystem::path dataFolder = "nvs1";
	if (!std::filesystem::exists(dataFolder)) {
		windowsError(L"The nvs1 folder containing the game data could not be found or is missing. Please obtain an original copy of the game or properly create the required nvs1 folder.", L"Data folder error ERROR367");
	}

	Console::Init();
	Logger::info("Console initalize edildi");

	//=========================================================
	//Window Init
	//=========================================================
	if (!window1.windowInit()) {
		Logger::error("Window olusturulamadi");
		return false;
	}

	int actualW = 0, actualH = 0;
	SDL_GetWindowSize(window1.getWindow(), &actualW, &actualH);
	windowWidth = actualW;
	windowHeight = actualH;
	Logger::info("Pencere boyutu: " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));

	//=========================================================
	//RTGL1 (Vulkan) Renderer Init
	//=========================================================
	// RTRenderer, RTGL1'in kendi Vulkan surface/swapchain yonetimini
	// SDL penceresinin HWND'i uzerinden kuruyor.
	if (!g_RTRenderer.Init(window1.getWindow(), windowWidth, windowHeight)) {
		Logger::error("RTRenderer initialize edilemedi.");
		return false;
	}
	Logger::info("RTRenderer initialize edildi.");

	//=========================================================
	// Bitmap Font
	//=========================================================
	// NOT: BitmapFont/UIWindow su an hala eski OpenGL FFP cizim kodunu
	// (glBegin/glTexImage2D) kullaniyor. RTGL1 devreye girdikten sonra bu
	// sistemlerin de rasterized-overlay (rgUploadNonWorldPrimitive) yoluna
	// tasinmasi gerekecek.
	/*if (!g_HudFont.Load("nvs1/gfx/hud_font.tga")) {
		Logger::error("HUD fontu yuklenemedi.");
	}

	if (!UIWindow::GBLoadIcon("nvs1/gfx/window_icon.tga")) {
		Logger::error("Pencere ikonu yuklenemedi.");
	}*/

	//=========================================================
	// Main Menu
	//=========================================================
	SDL_SetRelativeMouseMode(SDL_FALSE);
	MainMenu::Init();
	MainMenu::LoadBackgroundImage("nvs1/gfx/env/dusklandft.tga");

	const float btnX = 70.0f;
	const float btnW = 320.0f;
	const float btnH = 55.0f;
	const float btnSpacing = 68.0f;
	const float topMargin = 30.0f;

	MainMenu::AddButton(btnX, topMargin + btnSpacing * 0, btnW, btnH, "NEW GAME", []() {
		ReadEntityLump("nvs1/map/firstmap.bsp");
		LoadMap("firstmap");
		BotManager::Init();
		EnterPlaying();
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 1, btnW, btnH, "LOAD GAME", []() {
		Console::Log("Load game henuz baglanmadi");
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 2, btnW, btnH, "FIND SERVERS", []() {
		ServerMenu::Open();
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 3, btnW, btnH, "NEW MULTIPLAYER GAME", []() {
		Console::Log("new multiplayer game henuz baglanmadi");
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 4, btnW, btnH, "HOW TO PLAY", []() {
		Console::Log("How to play henuz baglanmadi");
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 5, btnW, btnH, "SETTINGS", []() {
		Settings::Open();
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 6, btnW, btnH, "QUIT", []() {
		SDL_Event quitEvent;
		quitEvent.type = SDL_QUIT;
		SDL_PushEvent(&quitEvent);
		});

	PauseMenu::Init();
	ServerMenu::Init();

	//=========================================================
	// Enet
	//=========================================================
	if (enet_initialize() != 0) {
		Logger::error("ENet baslatilamadi.");
	}
	else {
		Logger::info("ENet baslatildi.");
	}

	lastCounter = SDL_GetPerformanceCounter();
	Logger::info("Engine initalize edildi.");

	return true;

}

//=========================================================
//Game Loop
//=========================================================
void Engine::gameLoop() {
	while (running) {
		Uint64 currentCounter = SDL_GetPerformanceCounter();
		float deltaTime =
			static_cast<float>(currentCounter - lastCounter) /
			static_cast<float>(SDL_GetPerformanceFrequency());
		lastCounter = currentCounter;

		Time::Update(deltaTime);

		glm::vec3 oldPos = g_Camera.position;

		KeyInput::Update(running, g_Camera, deltaTime);
		Console::Update(deltaTime);
		NetClient::Update(g_CVar.nvs_gravity, g_CVar.nvs_jumpforce);

		std::string newMapName;
		if (NetClient::PollMapChange(newMapName)) {
			LoadMap(newMapName);
		}

		g_Camera.Update(deltaTime);
		if (g_State == GameState::Playing) {
			UpdatePlayerPhysics(deltaTime, oldPos);
			BotManager::Update(deltaTime);
		}
		else if (g_State == GameState::MenuLive) {
			MainMenu::Update(deltaTime);
		}

		RenderFrame();
	}
}


void Engine::RenderFrame() {
	g_RTRenderer.BeginFrame();

	// NOT: World geometrisi (g_MapRenderer), bot'lar ve skybox su an hala
	// eski OpenGL FFP cizim kodunu kullaniyor (glBegin/glVertex3f). Bu
	// cagrilar RTGL1'e hicbir sey "upload" etmiyor -- yani su an path-traced
	// bir goruntu ALMIYORUZ, sadece RTGL1'in bos bir frame'i "start/end"
	// etme dongusunu dogruluyoruz. BSPMapRenderer'in
	// DrawRenderFace fonksiyonunu glBegin/glVertex3f yerine
	// rgUploadMeshPrimitive kullanacak sekilde yeniden yazmak.
	if (g_State == GameState::MenuLive || g_State == GameState::Playing || g_State == GameState::Paused) {
		Frustum frustum = Frustum::FromViewProjection(glm::mat4(1.0f)); // gecici -- asagida acikliyorum
		(void)frustum;

		//g_MapRenderer.RenderWorld();
		//g_MapRenderer.RenderBrushEntities(g_Map.GetEntities());
		BotManager::Render();
	}

	const float fovYRadians = glm::radians(75.0f);
	g_RTRenderer.EndFrame(g_Camera.GetViewMatrix(), fovYRadians, 0.1f, 10000.0f);
	
	SDL_Delay(0); // Vulkan present zaten senkronize ediyor, ekstra swap cagrisi yok
}

void Engine::windowsError(const std::wstring& message, const std::wstring& title) {
	int rnvalue = MessageBoxW(NULL, message.c_str(), title.c_str(),
		MB_ICONERROR | MB_OK | MB_APPLMODAL | MB_TOPMOST);

		exit(EXIT_FAILURE);
}
