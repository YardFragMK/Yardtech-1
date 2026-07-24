#pragma once
#include"MapTypes.h"
#include<string>
#include<vector>

class MapParser {
public:
    bool Load(const std::string& path, MapData& out);

private:
    // Tokenizer
    std::vector<std::string> Tokenize(const std::string& text);

    // Parser durumları
    size_t m_pos = 0;
    std::vector<std::string> m_tokens;

    const std::string& Peek() const;
    std::string Next();
    bool Match(const std::string& tok);
    void Expect(const std::string& tok, const char* context);

    MapEntity ParseEntity();
    Brush ParseBrush();
    BrushFace ParseFace();
    glm::vec3 ParsePoint(); // "( x y z )"

    EntityKind ClassifyEntity(const std::string& classname) const;
};