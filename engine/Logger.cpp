#include<iostream>
#include"Logger.h"

void Logger::info(std::string itext) {
	std::cout << "[INFO] " << itext << std::endl;
}
void Logger::warning(std::string wtext) {
	std::cout << "[WARNING] " << wtext << std::endl;
}

void Logger::error(std::string etext) {
	std::cout << "[ERROR] " << etext << std::endl;
}