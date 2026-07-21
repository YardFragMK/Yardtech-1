#pragma once
#include"../Logger.h"
#include<vector>
#include<string>
#include<SDL.h>

class Console {
public:
	static void Init();
	static void Toggle();
	static void Shutdown();
	static bool IsOpen();
	static void Read();
	static void HandleEvent(const SDL_Event& event);

private:
	static void ExecuteCommand();
	static bool s_open;
	static std::string s_input;
};