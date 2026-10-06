#pragma once
#include <string>
#include <vector>
#include <cstdint>

bool LoadMap(const std::string& bspPath);
bool LoadMapFromMemory(const std::vector<uint8_t>& bspData, const std::string& mapName);