#pragma once
#include <string>
#include <unordered_map>

struct BitmapGlyph {
    float u0, v0, u1, v1;
    float pixelWidth;
    float pixelHeight;
};

class BitmapFont {
public:
    bool Load(const std::string& atlasTgaPath);
    bool IsLoaded() const { return !m_textureName.empty(); }

    float UIDrawText(float x, float y, const std::string& text, float pixelHeight,
        float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) const;

    float MeasureTextWidth(const std::string& text, float pixelHeight) const;

private:
    unsigned int m_texture;
    std::string m_textureName;
    std::unordered_map<char32_t, BitmapGlyph> m_glyphs;

    static void BuildGlyphTable(std::unordered_map<char32_t, BitmapGlyph>& glyphs, int atlasW, int atlasH);
};

extern BitmapFont g_HudFont;