#include"Console.h"
#include<iostream>


bool Console::s_open = false;
std::vector<ConsoleLine>Console::s_Lines;

void Console::Init() {
	Print("Yardtech Console");
	Print("Verion 1.0");
}

void Console::Toggle() {
	s_open = !s_open;
	if (s_open) {
		std::cout << "[Console opened] " << std::endl;
	}
	else {
		std::cout << "[Console closed] " << std::endl;
	}
}

void Console::Shutdown() {
	Print("Console Shutdown");
}  

bool Console::IsOpen() {
	return s_open;
}

void Console::Print(const std::string& text) {
	s_Lines.push_back({ text });
	std::cout << text << std::endl;
}

const std::vector<ConsoleLine>& Console::GetLines() {
	return s_Lines;
}
