#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <windows.h>
#include <GL/gl.h>

// Half-Life WAD3 texture paketi okuyucu.
// Bir .wad dosyasindaki tum miptex'leri decode edip
// isimle (uppercase) erisilebilir GL texture'lara cevirir.
class WadFile {
public:
    bool Load(const std::string& wadPath);

    // isim HL'de case-insensitive, biz hep uppercase karsilastiriyoruz
    GLuint GetTexture(const std::string& name, int* outWidth = nullptr, int* outHeight = nullptr) const;
    bool Contains(const std::string& name) const;

private:
    struct Entry {
        GLuint glTex = 0;
        int width = 0;
        int height = 0;
    };
    std::unordered_map<std::string, Entry> m_textures;

    static std::string ToUpper(const std::string& s);
};

// bpp=8 indexed data + 256 renkli palet -> RGBA8 buffer
// name '{' ile basliyorsa index 255 transparan kabul edilir (HL convention)
std::vector<uint8_t> DecodeIndexedToRGBA(const uint8_t* indices, int width, int height,
    const uint8_t* palette, bool useColorKey);

GLuint CreateGLTextureFromRGBA(const std::vector<uint8_t>& rgba, int width, int height);