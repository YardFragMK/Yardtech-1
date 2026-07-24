#pragma once
#include<glm/glm.hpp>
#include<vector>
#include<string>
#include<unordered_map>
#include<memory>

//=========================================================
// Bir brush yüzeyinin ham verisi (Valve 220 format)
//=========================================================
struct BrushFace {
    // Düzlemi tanımlayan 3 nokta (.map dosyasındaki ham koordinatlar, Z-up)
    glm::vec3 p0, p1, p2;

    // Düzlem denklemi (p0/p1/p2'den hesaplanır): dot(normal, point) = dist
    glm::vec3 normal{ 0.0f };
    float dist = 0.0f;

    std::string textureName;

    // Valve 220 texture eksenleri
    glm::vec3 uAxis{ 0.0f };
    float uOffset = 0.0f;
    glm::vec3 vAxis{ 0.0f };
    float vOffset = 0.0f;

    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    void ComputePlane() {
        normal = glm::normalize(glm::cross(p2 - p0, p1 - p0)); // DÜZELTİLDİ: sıra değişti
        dist = glm::dot(normal, p0);
    }
};

//=========================================================
// Bir brush = düzlemlerin kesişiminden oluşan convex hacim
//=========================================================
struct Brush {
    std::vector<BrushFace> faces;
};

//=========================================================
// Entity sınıflandırması
//=========================================================
enum class EntityKind {
    Worldspawn, // brush'ları taşıyan ana entity (harita geometrisinin kendisi)
    Light,
    Spawn,
    Door,
    Monster,
    Generic     // sınıflandırılmamış / tanınmayan classname
};

struct MapEntity {
    EntityKind kind = EntityKind::Generic;
    std::string classname;
    std::unordered_map<std::string, std::string> keyValues;
    std::vector<Brush> brushes; // point entity ise boş kalır

    // Sık kullanılan alanlar için yardımcılar
    glm::vec3 GetOrigin() const {
        auto it = keyValues.find("origin");
        if (it == keyValues.end()) return glm::vec3(0.0f);
        float x, y, z;
        if (sscanf_s(it->second.c_str(), "%f %f %f", &x, &y, &z) == 3) {
            return glm::vec3(x, y, z);
        }
        return glm::vec3(0.0f);
    }

    float GetFloat(const std::string& key, float def = 0.0f) const {
        auto it = keyValues.find(key);
        if (it == keyValues.end()) return def;
        return std::stof(it->second);
    }

    std::string GetString(const std::string& key, const std::string& def = "") const {
        auto it = keyValues.find(key);
        return (it != keyValues.end()) ? it->second : def;
    }
};

struct MapData {
    std::vector<MapEntity> entities;

    // worldspawn'ı bulmak için kısayol (genelde entities[0]'dır ama garanti değil)
    const MapEntity* GetWorldspawn() const {
        for (const auto& e : entities) {
            if (e.kind == EntityKind::Worldspawn) return &e;
        }
        return nullptr;
    }
};