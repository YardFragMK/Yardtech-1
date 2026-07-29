#pragma once
#include"../../engine/console/Console.h"
#include<string>

class e_PolyBase {
public:
	std::string name = "Unknown";
	int health = 100;
	virtual ~e_PolyBase() = default;
	virtual void Damage() {}
	virtual void Break() {}
};