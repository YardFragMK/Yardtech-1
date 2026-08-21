#include<iostream>
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
#include "../game/src/EntityParser.h"

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

	return true;

}

//=========================================================
//Game Loop
//=========================================================
void Engine::gameLoop() {
	//=========================================================
    // DELTATIME
	//=========================================================
	while (running) {
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


		//=========================================================
		// Collision / Hareket (noclip kapaliyken)
		//=========================================================
		if (!g_CVar.cm_noclip) {
			if (!Console::IsOpen()) {
				const Uint8* keys = SDL_GetKeyboardState(nullptr);
				bool ctrlHeld = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];

				constexpr float HALF_STAND = 36.0f;   // hull1 yari-yukseklik
				constexpr float HALF_CROUCH = 18.0f;  // hull3 yari-yukseklik
				constexpr float HULL_DIFF = HALF_STAND - HALF_CROUCH; // 18

				int currentHull = g_Camera.isCrouching ? 3 : 1;
				glm::vec3 groundCheckEnd = oldPos - glm::vec3(0.0f, 2.0f, 0.0f);
				TraceResult groundTrace = g_Map.TraceLine(oldPos, groundCheckEnd, currentHull);
				g_Camera.onGround = (groundTrace.fraction < 1.0f);

				if (g_Camera.onGround && g_Camera.verticalVelocity <= 0.0f) {
					g_Camera.verticalVelocity = 0.0f;
				}

				// --- ziplama ---
				if (keys[SDL_SCANCODE_SPACE] && g_Camera.onGround) {
					g_Camera.verticalVelocity = g_CVar.nvs_jumpforce;
					g_Camera.onGround = false;
				}

				// --- CTRL: havadaysa sert inis yerdeyse egilme ---
				if (ctrlHeld && !g_Camera.onGround && g_Player.RGDitem) {
					g_Camera.verticalVelocity = -g_CVar.nvs_jumpforce * 4.0f;
				}
				else if (g_Camera.onGround) {
					if (ctrlHeld && !g_Camera.isCrouching) {
						// egilmeye BASLA: origin'i asagi kaydir, ayaklar ayni yukseklikte kalsin
						g_Camera.position.y -= HULL_DIFF;
						oldPos.y -= HULL_DIFF;
						g_Camera.isCrouching = true;
					}
					else if (!ctrlHeld && g_Camera.isCrouching) {
						// --- kalkmaya BASLA ---
						// kalksaydik nereye giderdik, o NOKTA solid mi diye dogrudan kontrol et
						// (trace degil, cunku trace baslangici hala eski-hull uzayinda olurdu ve yanlis sonuc verirdi)
						glm::vec3 candidateStandPos = g_Camera.position + glm::vec3(0.0f, HULL_DIFF, 0.0f);
						if (!g_Map.IsPointSolid(candidateStandPos, 1)) {
							g_Camera.position = candidateStandPos;
							oldPos.y += HULL_DIFF;
							g_Camera.isCrouching = false;
						}
						// solid ise kalkamiyoruz, egik kalmaya devam
					}
				}

				int moveHull = g_Camera.isCrouching ? 3 : 1;

				// --- yercekimi entegrasyonu (semi-implicit Euler) --
				g_Camera.verticalVelocity -= g_CVar.nvs_gravity * deltaTime;
				g_Camera.position.y += g_Camera.verticalVelocity * deltaTime;

				// --- hareketi collision ile coz (duvarda/zeminde kayarak ilerleme) ---
				glm::vec3 newPos = g_Camera.position;
				if (newPos != oldPos) {
					glm::vec3 resolvedPos = g_Map.SlideMove(oldPos, newPos, moveHull);
					g_Camera.position = resolvedPos;

					if (std::abs(resolvedPos.y - newPos.y) > 0.01f) {
						g_Camera.verticalVelocity = 0.0f;
					}
				}
			}
		}
		else {
			// noclip acik: ucus modu, yercekimi/collision/crouch devre disi
			g_Camera.verticalVelocity = 0.0f;
			g_Camera.isCrouching = false;
		}

		//=========================================================
		// Render
		//=========================================================
		renderer.BeginFrame(g_Camera);

		Frustum frustum = Frustum::FromViewProjection(
			renderer.GetProjectionMatrix() * renderer.GetViewMatrix()
		);

		g_Map.RenderWorld(frustum);
		g_Map.RenderBrushEntities(frustum);

		renderer.EndFrame();
		Console::Render(windowWidth, windowHeight);

		SDL_GL_SwapWindow(window1.getWindow());
	}
}
