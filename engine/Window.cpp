#include"Window.h"
#include "Logger.h"

SDL_Window* Window::getWindow() const {
	return window;
}

bool Window::windowInit() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("SDL video system initalize edildi.");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	window = SDL_CreateWindow(
		"Yardtech 1",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		windowWidth,
		windowHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	if (!window) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("window olusturuldu.");

	return true;
}