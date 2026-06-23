#pragma once 

#include<string>


class CVar {
public:
	CVar() = default;
	CVar(const std::string& name,
		const std::string& value);
	std::string name;
	std::string value;

private:



};