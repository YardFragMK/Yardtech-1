#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdlib>

// ---------------------------------------------------------
// Entity temsili: ham key-value harita
// ---------------------------------------------------------
struct Entity{
    std::map<std::string, std::string> keyValues;

    const std::string* Get(const std::string& key) const{
        auto it = keyValues.find(key);
        return (it != keyValues.end()) ? &it->second : nullptr;
    }

    bool Is(const std::string& classnameToCheck) const{
        const std::string* cn = Get("classname");
        return cn && *cn == classnameToCheck;
    }

    // Yardimci: sayisal deger okuma (orn. "material" "1" -> 1)
    int GetInt(const std::string& key, int defaultValue = 0) const{
        const std::string* v = Get(key);
        return v ? std::atoi(v->c_str()) : defaultValue;
    }

    float GetFloat(const std::string& key, float defaultValue = 0.0f) const{
        const std::string* v = Get(key);
        return v ? static_cast<float>(std::atof(v->c_str())) : defaultValue;
    }
};

// Ham entity metnini (BSPReader::ReadEntityLump ciktisi) parse edip
// Entity listesine cevirir.
std::vector<Entity> ParseEntities(const std::string& entityText);


namespace EntityKeys
{
    constexpr const char* Classname = "classname";
    constexpr const char* Origin = "origin";
    constexpr const char* Model = "model";
    constexpr const char* Angle = "angle";
    constexpr const char* Angles = "angles";
    constexpr const char* Spawnflags = "spawnflags";
    constexpr const char* Targetname = "targetname";
    constexpr const char* Target = "target";
    constexpr const char* RenderMode = "rendermode";
    constexpr const char* RenderAmt = "renderamt";
    constexpr const char* RenderColor = "rendercolor";
    constexpr const char* Health = "health";

    // poly_breakable
    constexpr const char* Material = "material";
    constexpr const char* Gibmodel = "gibmodel";
    constexpr const char* Explosion = "explosion";
}

// ---------------------------------------------------------
// "classname" isimleri
// ---------------------------------------------------------
namespace EntityClassnames
{
    constexpr const char* Worldspawn = "worldspawn";
    constexpr const char* PolyBreakable = "poly_breakable";
    constexpr const char* PolyDoor = "poly_door";
    constexpr const char* PolyWall = "poly_wall";
    constexpr const char* PlayerStart = "player_start";
    constexpr const char* LightEntity = "light";
}

// ---------------------------------------------------------
// poly_breakable "material" alaninin deger isimleri
// ---------------------------------------------------------
enum class BreakableMaterial {
    Glass = 0,
    Wood = 1,
    Metal = 2,
    Flesh = 3,
    CinderBlock = 4,
    CeilingTile = 5,
    Computer = 6,
    UnbreakableGlass = 7,
    Rocks = 8
};