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
	static void HandleEvent(const SDL_Event& event);

	static void Update(float deltaTime);
	static void Render(int windowWidth, int windowHeight);
	static void Log(const std::string& line);

private:
	static void ExecuteCommand();
	static bool s_open;
	static std::string s_input;

	static std::vector<std::string> s_log; // komut ciktilari / gecmis
	static float s_currentHeight; // pikselde, o an ekranda kapladigi yukseklik
	static float s_targetHeight;  // acikken ulasmasi gereken hedef yukseklik
	static float s_slideSpeed;    // ne kadar hizli acilip kapanacak
};