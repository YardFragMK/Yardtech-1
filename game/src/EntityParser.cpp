#include "EntityParser.h"

std::vector<Entity> ParseEntities(const std::string& text)
{
    std::vector<Entity> entities;
    size_t i = 0;
    const size_t n = text.size();

    auto skipWhitespace = [&](size_t pos)
        {
            while (pos < n && (text[pos] == ' ' || text[pos] == '\n' ||
                text[pos] == '\r' || text[pos] == '\t'))
                ++pos;
            return pos;
        };

    while (i < n)
    {
        i = skipWhitespace(i);
        if (i >= n) break;

        if (text[i] == '{')
        {
            Entity ent;
            ++i;

            while (i < n)
            {
                i = skipWhitespace(i);
                if (i >= n) break;

                if (text[i] == '}') { ++i; break; }

                if (text[i] == '"')
                {
                    // key
                    size_t keyStart = ++i;
                    while (i < n && text[i] != '"') ++i;
                    std::string key = text.substr(keyStart, i - keyStart);
                    ++i; // kapanis "

                    i = skipWhitespace(i);

                    // value
                    size_t valStart = ++i; // acilis " atla
                    while (i < n && text[i] != '"') ++i;
                    std::string value = text.substr(valStart, i - valStart);
                    ++i; // kapanis "

                    ent.keyValues[key] = value;
                }
                else
                {
                    ++i; // beklenmeyen karakter, atla
                }
            }
            entities.push_back(std::move(ent));
        }
        else
        {
            ++i;
        }
    }

    return entities;
}