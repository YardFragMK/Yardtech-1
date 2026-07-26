#include "Renderer.h"

bool Renderer::Init(int width, int height) {
	glEnable(GL_DEPTH_TEST);
	SetupProjection(width, height);
	return true;
}

void Renderer::SetupProjection(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glm::mat4 projection = glm::perspective(
		glm::radians(75.0f), //FOV
		(float)width / (float)height,
		0.1f,   //near plane
		1000.0f  //far plane
	);

	glLoadMatrixf(glm::value_ptr(projection));
	glMatrixMode(GL_MODELVIEW);
}

void Renderer::BeginFrame(const Camera& camera) {
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLoadIdentity();
	glm::mat4 view = camera.GetViewMatrix();
	glLoadMatrixf(glm::value_ptr(view));
}

void Renderer::DrawTestTriangle() {
	glBegin(GL_TRIANGLES);
	glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0,-5.0);
	glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f, -1.0, -5.0);
	glTexCoord2f(0.5f,1.0f); glVertex3f(0.0f, 1.0, -5.0);
	glEnd();
}	


