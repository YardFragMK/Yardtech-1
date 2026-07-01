#pragma once
#include "PrimitiveRenderer.h"
#include "Mesh.h"

class Renderer {
public:
	static bool Init();
	static void BeginFrame();
	static void EndFrame();
	static void Shutdown();
private:
	static Mesh s_quad;
};