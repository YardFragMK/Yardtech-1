#include"MapParser.h"
#include<fstream>
#include<sstream>
#include<iostream>
#include<cctype>
#include<stdexcept>

bool MapParser::Load(const std::string& path, MapData& out) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "MAP acilamadi: " << path << std::endl;
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string text = ss.str();

    m_tokens = Tokenize(text);
    m_pos = 0;

    out.entities.clear();

    try {
        while (m_pos < m_tokens.size()) {
            if (Peek() == "{") {
                out.entities.push_back(ParseEntity());
            }
            else {
                Next(); // beklenmeyen token, atla (dosya sonu bosluklari vb.)
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "MAP parse hatasi: " << e.what() << std::endl;
        return false;
    }

    int totalBrushes = 0;
    for (auto& e : out.entities) totalBrushes += (int)e.brushes.size();

    std::cout << "MAP yuklendi: " << path << " (" << out.entities.size()
        << " entity, " << totalBrushes << " brush)" << std::endl;
    return true;
}

//=========================================================
// Tokenizer
//=========================================================
std::vector<std::string> MapParser::Tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    size_t i = 0;
    size_t n = text.size();

    while (i < n) {
        char c = text[i];

        // Bosluk/satir sonu atla
        if (std::isspace((unsigned char)c)) { i++; continue; }

        // Yorum satiri "//"
        if (c == '/' && i + 1 < n && text[i + 1] == '/') {
            while (i < n && text[i] != '\n') i++;
            continue;
        }

        // Tirnakli string (texture ismi bosluk icerebilir teorik olarak, ama
        // pratikte icermez; yine de tirnak varsa onu tek token olarak al)
        if (c == '"') {
            size_t start = ++i;
            while (i < n && text[i] != '"') i++;
            tokens.push_back(text.substr(start, i - start));
            i++; // kapanis tirnagini atla
            continue;
        }

        // Tek karakterlik ayirici token'lar
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']') {
            tokens.push_back(std::string(1, c));
            i++;
            continue;
        }

        // Diger her sey (sayilar, texture isimleri, classname vb.)
        size_t start = i;
        while (i < n && !std::isspace((unsigned char)text[i]) &&
            text[i] != '{' && text[i] != '}' &&
            text[i] != '(' && text[i] != ')' &&
            text[i] != '[' && text[i] != ']') {
            i++;
        }
        tokens.push_back(text.substr(start, i - start));
    }

    return tokens;
}

//=========================================================
// Token yardimcilari
//=========================================================
const std::string& MapParser::Peek() const {
    static const std::string empty = "";
    if (m_pos >= m_tokens.size()) return empty;
    return m_tokens[m_pos];
}

std::string MapParser::Next() {
    if (m_pos >= m_tokens.size()) throw std::runtime_error("beklenmedik dosya sonu");
    return m_tokens[m_pos++];
}

bool MapParser::Match(const std::string& tok) {
    if (Peek() == tok) { m_pos++; return true; }
    return false;
}

void MapParser::Expect(const std::string& tok, const char* context) {
    if (!Match(tok)) {
        throw std::runtime_error(std::string("beklenen '") + tok + "' bulunamadi (" + context + "), bulunan: '" + Peek() + "'");
    }
}

//=========================================================
// Entity parse
//=========================================================
MapEntity MapParser::ParseEntity() {
    Expect("{", "entity basi");

    MapEntity ent;

    while (Peek() != "}" && !Peek().empty()) {
        if (Peek() == "{") {
            // ic blok -> brush
            ent.brushes.push_back(ParseBrush());
        }
        else {
            // key-value cifti (iki tirnakli string)
            std::string key = Next();
            std::string value = Next();
            ent.keyValues[key] = value;
        }
    }

    Expect("}", "entity sonu");

    ent.classname = ent.GetString("classname");
    ent.kind = ClassifyEntity(ent.classname);

    return ent;
}

//=========================================================
// Brush parse
//=========================================================
Brush MapParser::ParseBrush() {
    Expect("{", "brush basi");

    Brush brush;
    while (Peek() != "}" && !Peek().empty()) {
        brush.faces.push_back(ParseFace());
    }

    Expect("}", "brush sonu");
    return brush;
}

//=========================================================
// Face parse (Valve 220 format):
// ( x1 y1 z1 ) ( x2 y2 z2 ) ( x3 y3 z3 ) TEXTURE [ ux uy uz uOff ] [ vx vy vz vOff ] rot scaleX scaleY
//=========================================================
BrushFace MapParser::ParseFace() {
    BrushFace face;

    face.p0 = ParsePoint();
    face.p1 = ParsePoint();
    face.p2 = ParsePoint();

    face.textureName = Next();

    // U ekseni: [ ux uy uz uOffset ]
    Expect("[", "u ekseni basi");
    face.uAxis.x = std::stof(Next());
    face.uAxis.y = std::stof(Next());
    face.uAxis.z = std::stof(Next());
    face.uOffset = std::stof(Next());
    Expect("]", "u ekseni sonu");

    // V ekseni: [ vx vy vz vOffset ]
    Expect("[", "v ekseni basi");
    face.vAxis.x = std::stof(Next());
    face.vAxis.y = std::stof(Next());
    face.vAxis.z = std::stof(Next());
    face.vOffset = std::stof(Next());
    Expect("]", "v ekseni sonu");

    face.rotation = std::stof(Next());
    face.scaleX = std::stof(Next());
    face.scaleY = std::stof(Next());

    face.ComputePlane();
    return face;
}

glm::vec3 MapParser::ParsePoint() {
    Expect("(", "nokta basi");
    float x = std::stof(Next());
    float y = std::stof(Next());
    float z = std::stof(Next());
    Expect(")", "nokta sonu");
    return glm::vec3(x, y, z);
}

//=========================================================
// Entity siniflandirma
//=========================================================
EntityKind MapParser::ClassifyEntity(const std::string& classname) const {
    if (classname == "worldspawn") return EntityKind::Worldspawn;

    if (classname == "light" || classname.rfind("light_", 0) == 0)
        return EntityKind::Light;

    if (classname == "info_player_start" || classname == "info_player_deathmatch")
        return EntityKind::Spawn;

    if (classname.rfind("func_door", 0) == 0)
        return EntityKind::Door;

    if (classname.rfind("monster_", 0) == 0)
        return EntityKind::Monster;

    return EntityKind::Generic;
}