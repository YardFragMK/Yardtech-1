#include "TGALoader.h"
#include <fstream>
#include <cstring>
#include "console/Console.h"

#pragma pack(push, 1)
struct TGAHeader {
    uint8_t idLength;
    uint8_t colorMapType;
    uint8_t imageType;
    uint8_t colorMapSpec[5];
    uint16_t xOrigin;
    uint16_t yOrigin;
    uint16_t width;
    uint16_t height;
    uint8_t pixelDepth;
    uint8_t imageDescriptor;
};
#pragma pack(pop)

bool LoadTGA(const std::string& path, std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        Console::Log("WARNING-> tga acilamadi: " + path);
        return false;
    }

    TGAHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(TGAHeader));
    if (!file) {
        Console::Log("WARNING-> tga header okunamadi: " + path);
        return false;
    }
    if (header.idLength > 0) file.seekg(header.idLength, std::ios::cur);

    if (header.imageType != 2 && header.imageType != 10) {
        Console::Log("WARNING-> desteklenmeyen tga tipi: " + path);
        return false;
    }
    if (header.pixelDepth != 24 && header.pixelDepth != 32) {
        Console::Log("WARNING-> desteklenmeyen tga bit derinligi: " + path);
        return false;
    }

    int width = header.width;
    int height = header.height;
    int bpp = header.pixelDepth / 8;
    size_t pixelCount = static_cast<size_t>(width) * height;

    std::vector<uint8_t> raw(pixelCount * bpp);

    if (header.imageType == 2) {
        file.read(reinterpret_cast<char*>(raw.data()), raw.size());
        if (!file) {
            Console::Log("WARNING-> tga veri okunamadi: " + path);
            return false;
        }
    }
    else {
        size_t pixelIndex = 0;
        std::vector<uint8_t> pixelBuf(bpp);
        while (pixelIndex < pixelCount) {
            uint8_t packetHeader = 0;
            file.read(reinterpret_cast<char*>(&packetHeader), 1);
            if (!file) break;

            int count = (packetHeader & 0x7F) + 1;
            bool rle = (packetHeader & 0x80) != 0;

            if (rle) {
                file.read(reinterpret_cast<char*>(pixelBuf.data()), bpp);
                for (int i = 0; i < count && pixelIndex < pixelCount; i++, pixelIndex++) {
                    std::memcpy(&raw[pixelIndex * bpp], pixelBuf.data(), bpp);
                }
            }
            else {
                for (int i = 0; i < count && pixelIndex < pixelCount; i++, pixelIndex++) {
                    file.read(reinterpret_cast<char*>(&raw[pixelIndex * bpp]), bpp);
                }
            }
        }
    }

    outPixels.assign(pixelCount * 4, 255);
    bool topToBottom = (header.imageDescriptor & 0x20) != 0;

    for (int y = 0; y < height; y++) {
        int srcRow = topToBottom ? y : (height - 1 - y);
        for (int x = 0; x < width; x++) {
            size_t srcIdx = (static_cast<size_t>(srcRow) * width + x) * bpp;
            size_t dstIdx = (static_cast<size_t>(y) * width + x) * 4;

            outPixels[dstIdx + 0] = raw[srcIdx + 2]; // TGA = BGR(A)
            outPixels[dstIdx + 1] = raw[srcIdx + 1];
            outPixels[dstIdx + 2] = raw[srcIdx + 0];
            outPixels[dstIdx + 3] = (bpp == 4) ? raw[srcIdx + 3] : 255;
        }
    }

    outWidth = width;
    outHeight = height;
    return true;
}