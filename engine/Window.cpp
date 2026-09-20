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

	window = SDL_CreateWindow(
		"Goblet of Blood Yardtech-1",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		windowWidth,
		windowHeight,
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

	if (!window) {
		Logger::error(SDL_GetError());
		return false;
	}
	Logger::info("window olusturuldu.");

	return true;
}