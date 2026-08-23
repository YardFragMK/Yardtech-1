#pragma once
#include <string>
#include <vector>
#include <cstdint>

// 24/32-bit TGA (uncompressed veya RLE) yukler. Cikti her zaman RGBA8, satirlar UST-SOL sirali.
bool LoadTGA(const std::string& path, std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight);