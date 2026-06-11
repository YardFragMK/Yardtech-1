#pragma once
#include<string>
class Logger {
public:
	void info(std::string itext);
	void warning(std::string wtext);
	void error(std::string etext);
};