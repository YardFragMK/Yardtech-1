#include "BitmapFont.h"
#include "TGALoader.h"
#include <vector>
#include "GLExtensions.h"

BitmapFont g_HudFont;

// UTF-8 baytlarini codepoint dizisine cevirir. Turkce harfler (Ğ, İ, Ş, Ö, Ü gibi)
// iki baytlik UTF-8 dizileri olarak gelir, bu yuzden duz char yerine char32_t
// anahtarli bir tabloya ihtiyac var.
static std::u32string DecodeUTF8(const std::string& s) {
    std::u32string out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp = 0;
        int extra = 0;

        if ((c & 0x80) == 0) { cp = c; extra = 0; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { i++; continue; }

        i++;
        bool valid = true;
        for (int k = 0; k < extra; k++) {
            if (i >= s.size() || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
            i++;
        }
        if (valid) out.push_back(cp);
    }
    return out;
}

// Atlas 1080x1080. Koordinatlar goruntunun alfa kanali taranarak cikarildi.
// Uc satir: buyuk harfler, ikinci satirdaki rakamlar/harfler, ucuncu satirdaki
// rakamlar ve islem sembolleri.
void BitmapFont::BuildGlyphTable(std::unordered_map<char32_t, BitmapGlyph>& glyphs, int atlasW, int atlasH) {
    struct RawGlyph { char32_t code; int x, y, w, h; };

    static const RawGlyph raw[] = {
         { U'A', 0, 13, 48, 55 },   { U'B', 68, 13, 44, 54 },   { U'C', 131, 13, 44, 54 },
         { U'D', 196, 13, 45, 54 }, { U'E', 264, 13, 37, 54 },  { U'F', 325, 13, 36, 54 },
         { U'G', 381, 13, 47, 54 }, { 0x011E, 449, 0, 48, 67 }, // Ğ
         { U'H', 520, 13, 46, 54 }, { U'I', 591, 13, 17, 55 },
         { 0x0130, 632, 0, 18, 68 }, // İ
         { U'J', 669, 13, 38, 54 }, { U'K', 732, 13, 47, 54 },  { U'L', 799, 13, 36, 54 },
         { U'M', 857, 13, 54, 54 }, { U'N', 936, 13, 45, 54 },  { U'O', 1004, 13, 51, 54 },

         { 0x00D6, 2, 108, 52, 71 }, // Ö
         { U'P', 76, 124, 43, 55 },  { U'R', 142, 124, 45, 55 }, { U'S', 205, 124, 43, 55 },
         { 0x015E, 267, 125, 43, 73 }, // Ş
         { U'T', 327, 124, 44, 55 }, { U'U', 393, 124, 45, 54 },
         { 0x00DC, 462, 109, 46, 70 }, // Ü
         { U'V', 527, 124, 49, 55 }, { U'Y', 590, 124, 45, 55 }, { U'Z', 651, 124, 42, 55 },
         { U'W', 710, 124, 67, 55 }, { U'Q', 794, 124, 55, 70 }, { U'X', 864, 124, 46, 55 },
         { U'1', 931, 125, 37, 54 }, { U'2', 988, 125, 40, 54 },

         { U'3', 1, 237, 40, 53 },   { U'4', 60, 237, 43, 53 },  { U'5', 121, 237, 38, 53 },
         { U'6', 181, 237, 39, 53 }, { U'7', 240, 237, 40, 53 }, { U'8', 300, 237, 40, 53 },
         { U'9', 358, 237, 41, 53 }, { U'0', 419, 237, 40, 53 },
         { U'+', 479, 242, 39, 41 },
         { U'x', 538, 245, 38, 37 }, // carpma sembolu icin kullanilan glyph
         { U'/', 597, 241, 40, 44 }, // bolme sembolu icin kullanilan glyph
         { U'>', 657, 243, 40, 38 }, { U'<', 717, 243, 39, 38 },
    };


    for (const auto& g : raw) {
        BitmapGlyph bg;
        bg.u0 = static_cast<float>(g.x) / atlasW;
        bg.v0 = static_cast<float>(g.y) / atlasH;
        bg.u1 = static_cast<float>(g.x + g.w) / atlasW;
        bg.v1 = static_cast<float>(g.y + g.h) / atlasH;
        bg.pixelWidth = static_cast<float>(g.w);
        bg.pixelHeight = static_cast<float>(g.h);
        glyphs[g.code] = bg;
    }
}

bool BitmapFont::Load(const std::string& atlasTgaPath) {
    std::vector<uint8_t> pixels;
    int w = 0, h = 0;
    if (!LoadTGA(atlasTgaPath, pixels, w, h)) return false;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);

    BuildGlyphTable(m_glyphs, w, h);
    return true;
}

static char32_t NormalizeCodepoint(char32_t cp) {
    // Atlasta sadece buyuk harfler var. ASCII kucuk harf gelirse buyuge cevir.
    // Turkce'ye ozgu i/I noktali-noktasiz kurallari kapsamiyor; cagiran taraf
    // Turkce metinleri zaten buyuk harfle gecirmeli.
    if (cp >= U'a' && cp <= U'z') return cp - 32;
    return cp;
}

float BitmapFont::DrawText(float x, float y, const std::string& text, float pixelHeight,
    float r, float g, float b, float a) const {
    if (m_texture == 0) return 0.0f;

    std::u32string codepoints = DecodeUTF8(text);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glColor4f(r, g, b, a);

    float cursorX = x;
    const float SPACE_ADVANCE = pixelHeight * 0.45f;
    const float LETTER_SPACING = pixelHeight * 0.08f;

    for (char32_t raw : codepoints) {
        char32_t cp = NormalizeCodepoint(raw);

        if (cp == U' ') {
            cursorX += SPACE_ADVANCE;
            continue;
        }

        auto it = m_glyphs.find(cp);
        if (it == m_glyphs.end()) {
            cursorX += SPACE_ADVANCE; // bilinmeyen karakter, bosluk kadar ilerle
            continue;
        }

        const BitmapGlyph& gl = it->second;
        float scale = pixelHeight / gl.pixelHeight;
        float drawW = gl.pixelWidth * scale;

        glBegin(GL_QUADS);
        glTexCoord2f(gl.u0, gl.v0); glVertex2f(cursorX, y);
        glTexCoord2f(gl.u1, gl.v0); glVertex2f(cursorX + drawW, y);
        glTexCoord2f(gl.u1, gl.v1); glVertex2f(cursorX + drawW, y + pixelHeight);
        glTexCoord2f(gl.u0, gl.v1); glVertex2f(cursorX, y + pixelHeight);
        glEnd();

        cursorX += drawW + LETTER_SPACING;
    }

    glDisable(GL_TEXTURE_2D);
    return cursorX - x;
}

float BitmapFont::MeasureTextWidth(const std::string& text, float pixelHeight) const {
    std::u32string codepoints = DecodeUTF8(text);
    float width = 0.0f;
    const float SPACE_ADVANCE = pixelHeight * 0.45f;
    const float LETTER_SPACING = pixelHeight * 0.08f;

    for (char32_t raw : codepoints) {
        char32_t cp = NormalizeCodepoint(raw);
        if (cp == U' ') { width += SPACE_ADVANCE; continue; }

        auto it = m_glyphs.find(cp);
        if (it == m_glyphs.end()) { width += SPACE_ADVANCE; continue; }

        float scale = pixelHeight / it->second.pixelHeight;
        width += it->second.pixelWidth * scale + LETTER_SPACING;
    }
    return width;
}