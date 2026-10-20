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
#include"BSPReader.h"
#include"BSPMap.h"
#include"BSPFormat.h"
#include"Frustum.h"
#include"console/CVar.h"
#include"../game/src/Player.h"
#include"EntityParser.h"
#include"PlayerMovement.h"
#include"viewsetup.h"
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
#include "ModuleLoader.h"
#include "Level.h"
#include "LocalGame.h"
#include "EntityRenderer.h"
#include"yg_ygui.h"

Renderer renderer;
BSPMap g_Map;
VirtualFileSystem vfs;

Engine::~Engine() {
	NetClient::Disconnect();
	enet_deinitialize();
	UnloadLevel();
	UnloadGameModules();
	YGUI::Shutdown();
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
	vfs.MountYPAK("nvs1/zhlpak.ypak"); //Test files
	vfs.MountDirectory("nvs1/");

	//=========================================================
	//Renderer Init
	//=========================================================
	if (!renderer.Init(windowWidth, windowHeight)) {
		Logger::error("Renderer initialize edilemedi.");
		return false;
	}
	Logger::info("Renderer initialize edildi.");

	//=========================================================
	// YGUI (arayuz kutuphanesi)
	//=========================================================
	YGUI::Initialize();
	Logger::info("YGUI initialize edildi.");

	//=========================================================
	// Oyun modulleri (client.dll / server.dll)
	//=========================================================
	// Moduller pencere, GL context ve VFS hazir olduktan sonra yuklenir;
	// client.dll ileride Init sirasinda GL kaynaklari olusturacaktir.
	if (!LoadGameModules("nvs1")) {
		Logger::error("Oyun modulleri tam olarak yuklenemedi (client.dll/server.dll).");
	}

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

	MainMenu::AddButton(btnX, topMargin + btnSpacing * 0, btnW, btnH, "NEW GAME", []() {
		// Basarisiz yuklemede LoadLevel hatayi kendisi loglar; oyun durumuna
		// yalnizca harita basariyla yuklendiyse gecilir. Botlar server.dll'in
		// LevelInit'inde haritadaki entity'lerden olusturulur.
		if (LoadLevel("firstmap")) {
			EnterPlaying();
		}
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
// Sabit tick: client.dll komutu uretir ve yerel oyuncuya uygular; ardindan
// komut ya uzak sunucuya (ENet) ya da yerel server.dll'e iletilir.
//=========================================================
static void RunFixedTick(float frameTime) {
	IBaseClientDLL* client = GetClientDLL();
	if (client == nullptr) return;

	PlayerInputCommand cmd;
	client->CreateMove(frameTime, &cmd);

	const bool playing = (g_State == GameState::Playing);
	const bool connected = NetClient::IsConnected();

	// Sunucudan gelen onayli durum, bu tick'in tahmininden once uygulanir.
	if (playing && connected && NetClient::HasReconciledState()) {
		client->ReceivePlayerState(NetClient::GetReconciledState(), 0);
	}

	client->RunCommand(cmd, frameTime);

	if (!playing) return;

	if (connected) {
		// Girdi oyuna ait degilken (konsol vb.) sunucuya komut gonderilmez.
		if (!cmd.consoleOpen) {
			NetClient::SendInputCommand(cmd, frameTime);
		}
	}
	else {
		// Yerel oyun mantigi (server.dll) yalnizca bir sunucuya bagli degilken
		// calisir; baglandiginda otoriter simulasyon uzaktaki sunucudadir.
		ViewSetup view;
		client->GetView(&view);
		LocalGame_Tick(frameTime, cmd.actions, view.origin, cmd.moveSpeed);
	}
}

//=========================================================
//Game Loop
//=========================================================
void Engine::gameLoop() {
	const float FIXED_DELTA_TIME = 1.0f / 100.0f; // Fizik ve kamera 100 Hz'de calissin
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

		//=========================================================
		// Input / Update
		//=========================================================
		Console::Update(deltaTime);
		NetClient::Update(g_CVar.nvs_gravity, g_CVar.nvs_jumpforce);

		// Sunucu haritayi degistirdiyse, client kendi (render+collision)
		// kopyasini da guncellemeli. Uzak sunucu oyunu yonettigi icin yerel
		// server.dll baslatilmaz.
		std::string newMapName;
		if (NetClient::PollMapChange(newMapName)) {
			LoadLevel(newMapName, false);
		}

		// Olaylar frame basina bir kez islenir (sabit tick'lerin icinde degil):
		// yuksek FPS'te tick calismayan frame'lerde olaylar beklemez.
		KeyInput::Update(running);

		if (IBaseClientDLL* client = GetClientDLL()) {
			client->Update(deltaTime);
		}

		//=========================================================
		// FIXED UPDATE DONGUSU (kamera ve fizik)
		//=========================================================
		while (accumulator >= FIXED_DELTA_TIME) {
			RunFixedTick(FIXED_DELTA_TIME);
			accumulator -= FIXED_DELTA_TIME;
		}

		float alpha = accumulator / FIXED_DELTA_TIME;

		if (g_State == GameState::MenuLive) {
			MainMenu::Update(deltaTime);
		}

		RenderFrame(alpha, deltaTime);
	}
}


void Engine::RenderFrame(float alpha, float deltaTime) {
	// Kamera client.dll'indir; MenuLive'da yalnizca bakis acilari menu
	// kamerasindan gelir.
	ViewSetup view;
	if (IBaseClientDLL* client = GetClientDLL()) {
		client->GetView(&view);
	}
	if (g_State == GameState::MenuLive) {
		MainMenu::ApplyLiveView(view);
	}

	renderer.BeginFrame(view);

	if (g_State == GameState::MenuLive || g_State == GameState::Playing || g_State == GameState::Paused) {
		g_Skybox.Render(view.origin);

		Frustum frustum = Frustum::FromViewProjection(
			renderer.GetProjectionMatrix() * renderer.GetViewMatrix()
		);

		g_MapRenderer.RenderWorld(frustum);
		g_MapRenderer.RenderBrushEntities(g_Map.GetEntities(), frustum);
		EntityRenderer_Render(LocalGame_GetEntities());

		if (IBaseClientDLL* client = GetClientDLL()) {
			client->RenderScene();
		}
	}

	renderer.EndFrame();


	if (g_State == GameState::Playing || g_State == GameState::Paused) {
		HUD::Render(windowWidth, windowHeight);
	}


	if (g_State == GameState::Paused) {
		PauseMenu::Render(windowWidth, windowHeight);
	}
	else if (g_State != GameState::Playing) {
		MainMenu::Render(windowWidth, windowHeight);
	}

	// YGUI karesi: HUD cizim komutlari uretir, EndFrame bunlari ekrana basar.
	// Henuz YGUI'ye gecmemis (FFP) menuler HUD'un USTUNE cizilmeli ki pause
	// karartmasi HUD'u da karartsin; bu yuzden EndFrame hemen cagrilir.
	YGUI::NewFrame(deltaTime, static_cast<float>(windowWidth), static_cast<float>(windowHeight));
	Settings::Render(windowWidth, windowHeight);
	ServerMenu::Render(windowWidth, windowHeight);
	YGUI::EndFrame();

	// client.dll arayuzu (ileride YGUI) konsoldan once cizilir; konsol her
	// zaman en ustte kalir.
	if (IBaseClientDLL* client = GetClientDLL()) {
		client->RenderUI(windowWidth, windowHeight);
	}

	Console::Render(windowWidth, windowHeight);

	SDL_GL_SwapWindow(window1.getWindow());
}

void Engine::windowsError(const std::wstring& message, const std::wstring& title) {
	int rnvalue = MessageBoxW(NULL, message.c_str(), title.c_str(),
		MB_ICONERROR | MB_OK | MB_APPLMODAL | MB_TOPMOST);

	exit(EXIT_FAILURE);
}