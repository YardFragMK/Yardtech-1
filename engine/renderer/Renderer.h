#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include<glad/glad.h>
#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include"viewsetup.h"
#include"../console/Console.h"
#include"PostProcess.h"

class Renderer {
public:
	bool Init(int width, int height);
	void BeginFrame(const ViewSetup& view);
	void EndFrame();
	void DrawTestTriangle();

	void ToggleRetroMode() {
		m_retroMode = !m_retroMode;
	}
	void SetRetroMode(bool enable);
	bool IsRetroMode() const { return m_retroMode; }

	const glm::dmat4 GetProjectionMatrix() const { return m_lastProjection; }
	const glm::dmat4 GetViewMatrix() const { return m_lastView; }

	// Tam ekran efektleri (CRT, ruya, su alti, kara delik).
	PostProcess& GetPostProcess() { return m_postProcess; }

private:
	GLuint m_retroTexture = 0;
	bool m_retroMode = false;
	int m_windowWidth = 1920;
	int m_windowHeight = 1080;
	const int RETRO_WIDTH = 320;
	const int RETRO_HEIGHT = 240;

	glm::dmat4 m_lastProjection = glm::mat4(1.0f);
	glm::dmat4 m_lastView = glm::mat4(1.0f);
	ViewSetup m_lastViewSetup;
	PostProcess m_postProcess;

	void ApplyProjection(int width, int height, const ViewSetup& view);
};