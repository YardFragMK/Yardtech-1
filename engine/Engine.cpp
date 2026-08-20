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
#include"Frustum.h"
#include"console/CVar.h"
#include"../game/src/Player.h"

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
			if (!Console::IsOpen()) {
				// --- yer kontrolu: eski pozisyondan asagi kisa bir trace ---
				glm::vec3 groundCheckEnd = oldPos - glm::vec3(0.0f, 2.0f, 0.0f);
				TraceResult groundTrace = g_Map.TraceLine(oldPos, groundCheckEnd, 1);
				g_Camera.onGround = (groundTrace.fraction < 1.0f);

				if (g_Camera.onGround && g_Camera.verticalVelocity <= 0.0f) {
					g_Camera.verticalVelocity = 0.0f;
				}

				// --- ziplama ---
				const Uint8* keys = SDL_GetKeyboardState(nullptr);
				if (keys[SDL_SCANCODE_SPACE] && g_Camera.onGround) {
					g_Camera.verticalVelocity = g_CVar.nvs_jumpforce;
					g_Camera.onGround = false;
				}

				// --- CTRL ile sert iniş ve eğilme ---
				if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) &&
					!g_Camera.onGround && g_Player.RGDitem) {
					g_Camera.verticalVelocity = -g_CVar.nvs_jumpforce * 4.0f;
				}
				else {
					
				}

				// --- yercekimi entegrasyonu ---
				g_Camera.verticalVelocity -= g_CVar.nvs_gravity * deltaTime;
				g_Camera.position.y += g_Camera.verticalVelocity * deltaTime;
			}

			glm::vec3 newPos = g_Camera.position;
			if (newPos != oldPos) {
				glm::vec3 resolvedPos = g_Map.SlideMove(oldPos, newPos, 1);
				g_Camera.position = resolvedPos;

				// dikeyde carpisma oldu (tavan/zemin) -> verticalVelocity'yi sifirla,
				// yoksa bir sonraki frame hala eski (birikmis) hizla hareket etmeye calisir
				if (std::abs(resolvedPos.y - newPos.y) > 0.01f) {
					g_Camera.verticalVelocity = 0.0f;
				}
			}
		}
		else {
			g_Camera.verticalVelocity = 0.0f; // noclip: ucus modu, yercekimi yok
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
