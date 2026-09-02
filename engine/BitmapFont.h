#pragma once
#include <string>
#include <unordered_map>
#include <windows.h>
#include <GL/gl.h>

// Atlas icindeki tek bir glyph'in normalize UV kutusu ve orijinal piksel boyutu.
// Piksel boyutu, cizim sirasinda genislik/yukseklik oranini korumak icin kullanilir.
struct BitmapGlyph {
    float u0, v0, u1, v1;
    float pixelWidth;
    float pixelHeight;
};

class BitmapFont {
public:
    bool Load(const std::string& atlasTgaPath);
    bool IsLoaded() const { return m_texture != 0; }

    // text UTF-8 kodlu olabilir (Turkce karakterler dahil). pixelHeight, glyph'in
    // ekranda kaplayacagi dikey piksel yuksekligidir; genislik orana gore olceklenir.
    // Donus degeri metnin toplam genisligidir (hizalama/ortalama icin kullanilabilir).
    float DrawText(float x, float y, const std::string& text, float pixelHeight,
        float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) const;

    float MeasureTextWidth(const std::string& text, float pixelHeight) const;

private:
    GLuint m_texture = 0;
    std::unordered_map<char32_t, BitmapGlyph> m_glyphs;

    static void BuildGlyphTable(std::unordered_map<char32_t, BitmapGlyph>& glyphs, int atlasW, int atlasH);
};

extern BitmapFont g_HudFont;