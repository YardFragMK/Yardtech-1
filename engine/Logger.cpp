#include"Logger.h"
#include<windows.h>

void Logger::info(std::string itext) {
	std::string line = "[INFO] " + itext+ "\n";
	OutputDebugStringA(line.c_str());

}
void Logger::warning(std::string wtext) {
	std::string line = "[WARNING] " + wtext + "\n";
	OutputDebugStringA(line.c_str());
}

void Logger::error(std::string etext) {
	std::string line = "[ERROR] " + etext + "\n";
	OutputDebugStringA(line.c_str());
}