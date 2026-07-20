#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include<glad/glad.h>
#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include"../Camera.h"

class Renderer {
public:
	 bool Init(int width, int height);
	 void SetupProjection(int width, int height);
	 void BeginFrame(const Camera& camera);
	 void DrawTestTriangle();
private:

};