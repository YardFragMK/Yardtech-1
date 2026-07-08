#pragma once
#include"../Logger.h"
#include<vector>
#include<string>
#include"ConsoleLine.h"

class Console {
public:
	static void Init();
	static void Toggle();
	static void Shutdown();
	static bool IsOpen();
	static void Print(const std::string& text);
	static const std::vector<ConsoleLine>& GetLines();

private:
	static bool s_open;
	static std::vector<ConsoleLine>s_Lines;
};