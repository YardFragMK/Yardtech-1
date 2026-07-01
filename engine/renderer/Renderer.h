#pragma once
#include "Shader.h"

class Renderer {
public:
	static bool Init();
	static void BeginFrame();
	static void EndFrame();
	static void Shutdown();
private:
	static Shader s_testShader;
};