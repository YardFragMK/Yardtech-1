#include "Renderer.h"
#include "../GLExtensions.h"

bool Renderer::Init(int width, int height) {
	m_windowWidth = width;
	m_windowHeight = height;

	glEnable(GL_DEPTH_TEST);
	LoadGLExtensions();

	glGenTextures(1, &m_retroTexture);
	glBindTexture(GL_TEXTURE_2D, m_retroTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	ApplyProjection(width, height);
	return true;
}

void Renderer::ApplyProjection(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glm::mat4 projection = glm::perspective(
		glm::radians(75.0f), //FOV
		(float)width / (float)height,
		0.1f,   //near plane
		10000.0f  //far plane
	);

	m_lastProjection = projection;

	glLoadMatrixf(glm::value_ptr(projection));
	glMatrixMode(GL_MODELVIEW);
}

void Renderer::BeginFrame(const Camera& camera) {
	if (m_retroMode) {
		glViewport(0, 0, RETRO_WIDTH, RETRO_HEIGHT);
		ApplyProjection(RETRO_WIDTH, RETRO_HEIGHT);
	}
	else {
		glViewport(0, 0, m_windowWidth, m_windowHeight);
		ApplyProjection(m_windowWidth, m_windowHeight);
	}



	glClearColor(0.08f, 0.01f, 0.01f, 0.85f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLoadIdentity();

	glm::mat4 view = camera.GetViewMatrix();
	m_lastView = view;
	glLoadMatrixf(glm::value_ptr(view));
}

void Renderer::EndFrame(){
	if (m_retroMode) {
		glBindTexture(GL_TEXTURE_2D, m_retroTexture);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, RETRO_WIDTH, RETRO_HEIGHT, 0);

		glViewport(0, 0, m_windowWidth, m_windowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, m_windowWidth, 0, m_windowHeight, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_retroTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f((float)m_windowWidth, 0.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f((float)m_windowWidth, (float)m_windowHeight);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)m_windowHeight);
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();

	}
}

void Renderer::DrawTestTriangle() {
	glBegin(GL_TRIANGLES);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0, -5.0);
		glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f, -1.0, -5.0);
		glTexCoord2f(0.5f,1.0f); glVertex3f(0.0f, 1.0, -5.0);
	glEnd();
}

void Renderer::SetRetroMode(bool enable) {
	m_retroMode = enable;
	if (m_retroMode == true) {
		Console::Log("retromode has launched");
	}
	else if (m_retroMode == false) {
		Console::Log("retromode has been shut down");
	}
}


