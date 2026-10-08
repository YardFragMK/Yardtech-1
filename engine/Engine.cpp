#include<iostream>
#include<filesystem>
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
#include"NetClient.h"
#include"BSPMapRenderer.h"
#include"ServerMenu.h"
#include"UIWindow.h"
#include"../game/src/BotManager.h"

Renderer renderer;
BSPMap g_Map;
VirtualFileSystem vfs;

Engine::~Engine(){
	NetClient::Disconnect();
	enet_deinitialize();

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
	// Check the data files
	std::filesystem::path dataFolder = "nvs1/gbpak.ypak";
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
	//VFS
	//=========================================================
	vfs.MountYPAK("nvs1/gbpak.ypak");
	vfs.MountDirectory("nvs1/");

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
	std::vector<uint8_t> fontBuffer = vfs.ReadFile("gfx/hud_font.tga");
	if (fontBuffer.empty() || !g_HudFont.LoadFromMemory(fontBuffer)) {
		Logger::error("HUD fontu VFS (Paket/Klasor) uzerinden yuklenemedi.");
	}

	std::vector<uint8_t> iconBuffer = vfs.ReadFile("gfx/window_icon.tga");
	if (iconBuffer.empty() || !UIWindow::GBLoadIconFromMemory(iconBuffer)) {
		Logger::error("Pencere ikonu VFS (Paket/Klasor) uzerinden yuklenemedi.");
	}


	//=========================================================
	// Main Menu
	//=========================================================
	SDL_SetRelativeMouseMode(SDL_FALSE);
	MainMenu::Init();
	std::vector<uint8_t> bgBuffer = vfs.ReadFile("gfx/env/dusklandft.tga");
	if (bgBuffer.empty() || !MainMenu::LoadBackgroundImageFromMemory(bgBuffer)) {
		Logger::error("Ana menu arka plan gorseli VFS uzerinden yuklenemedi.");
	}


	const float btnX = 70.0f;
	const float btnW = 320.0f;
	const float btnH = 55.0f;
	const float btnSpacing = 68.0f;
	const float topMargin = 30.0f;

	MainMenu::AddButton(btnX, topMargin + btnSpacing * 0, btnW, btnH, "NEW GAME", [this]() {
		std::vector<uint8_t> mapBuffer = vfs.ReadFile("map/firstmap.bsp");
		if (LoadMapFromMemory(mapBuffer, "firstmap")) {
			BotManager::Init();
			EnterPlaying();
		}
		else {
			Logger::error("Harita yukleme basarisiz.");
		}
		BotManager::Init();
		EnterPlaying();
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing * 1, btnW, btnH, "LOAD GAME", []() {
		Console::Log("Load game henuz baglanmadi");
		});
	MainMenu::AddButton(btnX, topMargin + btnSpacing *2, btnW, btnH, "FIND SERVERS", []() {
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
	const float FIXED_DELTA_TIME = 1.0f / 100.0f; // Fizik ve kamera 100 Hz'de çalışsın
	float accumulator = 0.0f;

	while (running) {
		//=========================================================
		// DELTATIME
		//=========================================================
		Uint64 currentCounter = SDL_GetPerformanceCounter();
		float deltaTime =
			static_cast<float>(currentCounter - lastCounter) /
			static_cast<float>(SDL_GetPerformanceFrequency());
		lastCounter = currentCounter;
		if (deltaTime > 0.1f) deltaTime = 0.1f;

		Time::Update(deltaTime);
		accumulator += deltaTime;

		glm::vec3 oldPos = g_Camera.position;

		//=========================================================
        // Input / Update
        //=========================================================
		Console::Update(deltaTime);
		NetClient::Update(g_CVar.nvs_gravity, g_CVar.nvs_jumpforce);

		// Sunucu haritayi degistirdiyse, client kendi (render+collision)
		// kopyasini da guncellemeli. Bu kontrol NetClient::Update'in disinda,
		// gameLoop'un kendisinde yapiliyor cunku multiplayer sirasinda
		// Playing disindaki durumlarda (orn. Paused) da gecerli olmalidir.
		std::string newMapName;
		if (NetClient::PollMapChange(newMapName)) {
			std::vector<uint8_t> mapBuffer = vfs.ReadFile("map/" + newMapName + ".bsp");
			LoadMapFromMemory(mapBuffer, newMapName);
		}

		//=========================================================
		// FIXED UPDATE DÖNGÜSÜ (Kamera ve Fizik)
		//=========================================================
		while (accumulator >= FIXED_DELTA_TIME) {
			glm::vec3 oldPos = g_Camera.position;

			KeyInput::Update(running, g_Camera, FIXED_DELTA_TIME);
			g_Camera.Update(FIXED_DELTA_TIME);

			if (g_State == GameState::Playing) {
				UpdatePlayerPhysics(FIXED_DELTA_TIME, oldPos);
				BotManager::Update(FIXED_DELTA_TIME);
			}

			accumulator -= FIXED_DELTA_TIME;
		}

		float alpha = accumulator / FIXED_DELTA_TIME;

		if (g_State == GameState::MenuLive) {
			MainMenu::Update(deltaTime);
		}

		// Çizim fonksiyonuna bu alphayı gönderiyoruz:
		RenderFrame(alpha);
	}
}


void Engine::RenderFrame(float alpha) {
	renderer.BeginFrame(g_Camera);

	if (g_State == GameState::MenuLive || g_State == GameState::Playing || g_State == GameState::Paused) {
		g_Skybox.Render(g_Camera.GetEyePosition());

		Frustum frustum = Frustum::FromViewProjection(
			renderer.GetProjectionMatrix() * renderer.GetViewMatrix()
		);

		g_MapRenderer.RenderWorld(frustum);
		g_MapRenderer.RenderBrushEntities(g_Map.GetEntities(), frustum);
		BotManager::Render();
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
	ServerMenu::Render(windowWidth, windowHeight);
	Console::Render(windowWidth, windowHeight);

	SDL_GL_SwapWindow(window1.getWindow());
}

void Engine::windowsError(const std::wstring& message, const std::wstring& title) {
	int rnvalue = MessageBoxW(NULL, message.c_str(), title.c_str(),
		MB_ICONERROR | MB_OK | MB_APPLMODAL | MB_TOPMOST);

		exit(EXIT_FAILURE);
}
