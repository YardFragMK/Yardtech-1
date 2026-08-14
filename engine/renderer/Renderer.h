#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include<glad/glad.h>
#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include"../Camera.h"
#include"../console/Console.h"

class Renderer {
public:
	 bool Init(int width, int height);
	 void BeginFrame(const Camera& camera);
	 void EndFrame();
	 void DrawTestTriangle();

	 void ToggleRetroMode() {
		 m_retroMode = !m_retroMode;
	 }
	 void SetRetroMode(bool enable);
private:
	GLuint m_retroTexture = 0;
	bool m_retroMode = false;
	int m_windowWidth = 1920;
	int m_windowHeight = 1080;
	const int RETRO_WIDTH = 320;
	const int RETRO_HEIGHT = 240;

	void ApplyProjection(int width, int height);
};