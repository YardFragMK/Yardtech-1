#pragma once

class Renderer {
public:
	static bool Init(int w, int h);
	static void BeginFrame();
	static void EndFrame();
	static void Shutdown();
private:

};