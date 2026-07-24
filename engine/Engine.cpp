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
#include"map/MapParser.h"
#include"map/BrushMesh.h"
#include"map/VertexLighting.h"
#include"map/BVH.h"
#include"map/FrustumExtract.h"


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



	// ... initSystems() içinde, renderer.Init'ten sonra:

	MapData mapData;
	MapParser parser;
	if (parser.Load("nvs1/maps/test.map", mapData)) {
		std::vector<SceneLight> lights = VertexLighting::ExtractLights(mapData);
		Logger::info("Isik sayisi: " + std::to_string(lights.size()));

		const MapEntity* worldspawn = mapData.GetWorldspawn();
		std::vector<BVHPrimitive> primitives; // <- allMeshes yerine dogrudan BVHPrimitive olustur

		if (worldspawn) {
			for (const auto& brush : worldspawn->brushes) {
				BrushMesh mesh;
				if (BrushMeshBuilder::Build(brush, mesh)) {
					VertexLighting::ApplyLighting(mesh, lights);

					BVHPrimitive prim;
					prim.mesh = std::move(mesh);
					for (auto& face : prim.mesh.faces) {
						for (auto& v : face.vertices) {
							prim.bounds.Encapsulate(v.position);
						}
					}
					primitives.push_back(std::move(prim));
				}
			}
		}

		worldBVH.Build(std::move(primitives)); // artik Engine'in uyesi
		Logger::info("BVH insa edildi: " + std::to_string(worldBVH.GetPrimitives().size()) + " primitive");
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
		KeyInput::Update(running, g_Camera, dynamicLights, deltaTime);

		g_Camera.Update(deltaTime);
		renderer.BeginFrame(g_Camera);
		//renderer.DrawTestTriangle();

		dynamicLights.Update(deltaTime);          // isiklari yaslandır, sonenleri temizle
		dynamicLights.ApplyToWorld(worldBVH);     // etkilenen vertex'leri guncelle

		// Kamera view + projection matrisini birlestir
		glm::mat4 projection = glm::perspective(glm::radians(75.0f), (float)windowWidth / windowHeight, 0.1f, 1000.0f);
		glm::mat4 view = g_Camera.GetViewMatrix();
		Frustum frustum = ExtractFrustum(projection * view);

		std::vector<int> visibleIndices;
		worldBVH.QueryFrustum(frustum, visibleIndices);

		glEnable(GL_TEXTURE_2D);
		for (int idx : visibleIndices) {
			const auto& mesh = worldBVH.GetPrimitives()[idx].mesh;
			for (const auto& face : mesh.faces) {
				// TODO: texture binding (texture sistemi entegrasyonundan sonra)
				glBegin(GL_TRIANGLE_FAN);
				for (const auto& v : face.vertices) {
					glColor3f(v.color.r, v.color.g, v.color.b);
					glTexCoord2f(v.uv.x, v.uv.y);
					glVertex3f(v.position.x, v.position.y, v.position.z);
				}
				glEnd();
			}
		}


			

		SDL_GL_SwapWindow(window1.getWindow());


	}
}
