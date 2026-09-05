#include "WadFile.h"
#include <fstream>
#include <algorithm>
//#include "../engine/console/Console.h"

#pragma pack(push, 1)
struct WadHeader {
    char magic[4];      // "WAD3"
    int32_t numlumps;
    int32_t infotableofs;
};
struct WadLumpInfo {
    int32_t filepos;
    int32_t disksize;
    int32_t size;        // uncompressed
    int8_t  type;        // 0x43 = miptex
    int8_t  compression; // 0 = none
    int16_t padding;
    char    name[16];
};
struct WadMiptex {
    char name[16];
    uint32_t width;
    uint32_t height;
    uint32_t offsets[4]; // mip0..mip3, bu struct'in basindan itibaren offset
};
#pragma pack(pop)

constexpr int8_t WAD_TYPE_MIPTEX = 0x43;

std::string WadFile::ToUpper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}

std::vector<uint8_t> DecodeIndexedToRGBA(const uint8_t* indices, int width, int height,
    const uint8_t* palette, bool useColorKey) {
    std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
    for (int i = 0; i < width * height; i++) {
        uint8_t idx = indices[i];
        uint8_t r = palette[idx * 3 + 0];
        uint8_t g = palette[idx * 3 + 1];
        uint8_t b = palette[idx * 3 + 2];
        uint8_t a = 255;
        if (useColorKey && idx == 255) {
            r = g = b = 0;
            a = 0;
        }
        rgba[i * 4 + 0] = r;
        rgba[i * 4 + 1] = g;
        rgba[i * 4 + 2] = b;
        rgba[i * 4 + 3] = a;
    }
    return rgba;
}

GLuint CreateGLTextureFromRGBA(const std::vector<uint8_t>& rgba, int width, int height) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

bool WadFile::Load(const std::string& wadPath) {
    std::ifstream file(wadPath, std::ios::binary);
    if (!file) {
        //Console::Log("WARNING-> wad acilamadi: " + wadPath);
        return false;
    }

    WadHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(WadHeader));
    if (std::string(header.magic, 4) != "WAD3") {
        //Console::Log("WARNING-> gecersiz wad magic: " + wadPath);
        return false;
    }

    std::vector<WadLumpInfo> lumps(header.numlumps);
    file.seekg(header.infotableofs, std::ios::beg);
    file.read(reinterpret_cast<char*>(lumps.data()), sizeof(WadLumpInfo) * header.numlumps);

    for (const auto& lump : lumps) {
        if (lump.type != WAD_TYPE_MIPTEX) continue;

        std::vector<uint8_t> raw(lump.disksize);
        file.seekg(lump.filepos, std::ios::beg);
        file.read(reinterpret_cast<char*>(raw.data()), lump.disksize);

        if (raw.size() < sizeof(WadMiptex)) continue;
        WadMiptex mt{};
        std::memcpy(&mt, raw.data(), sizeof(WadMiptex));

        if (mt.offsets[0] == 0) continue; // veri yok

        const uint8_t* mip0 = raw.data() + mt.offsets[0];
        // mip0 + mip1 + mip2 + mip3 boyutlari toplaninca palet basliyor
        size_t mip0Size = static_cast<size_t>(mt.width) * mt.height;
        size_t mip1Size = mip0Size / 4;
        size_t mip2Size = mip0Size / 16;
        size_t mip3Size = mip0Size / 64;
        const uint8_t* paletteCountPos = mip0 + mip0Size + mip1Size + mip2Size + mip3Size;
        uint16_t paletteCount = 0;
        std::memcpy(&paletteCount, paletteCountPos, sizeof(uint16_t));
        const uint8_t* palette = paletteCountPos + sizeof(uint16_t);

        std::string name(mt.name);
        bool colorKey = !name.empty() && name[0] == '{';

        auto rgba = DecodeIndexedToRGBA(mip0, mt.width, mt.height, palette, colorKey);
        GLuint tex = CreateGLTextureFromRGBA(rgba, mt.width, mt.height);

        Entry e;
        e.glTex = tex;
        e.width = mt.width;
        e.height = mt.height;
        e.isMasked = colorKey;
        m_textures[ToUpper(name)] = e;
    }

    //Console::Log(wadPath + " yuklendi (" + std::to_string(m_textures.size()) + " texture)");
    return true;
}

GLuint WadFile::GetTexture(const std::string& name, int* outWidth, int* outHeight) const {
    auto it = m_textures.find(ToUpper(name));
    if (it == m_textures.end()) return 0;
    if (outWidth) *outWidth = it->second.width;
    if (outHeight) *outHeight = it->second.height;
    return it->second.glTex;
}

bool WadFile::Contains(const std::string& name) const {
    return m_textures.find(ToUpper(name)) != m_textures.end();
}

bool WadFile::IsMasked(const std::string& name) const {
    auto it = m_textures.find(ToUpper(name));
    return (it != m_textures.end()) && it->second.isMasked;
}