#pragma once
#include<string>
class Logger {
public:
	static void info(std::string itext);
	static void warning(std::string wtext);
	static void error(std::string etext);
};