#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "netentity.h"

// Mermi isiklari gibi gecici dinamik isiklarin her kare toplandigi liste. Dunya
// shader'i (worldlit) en yakin MAX_SHADER_LIGHTS isigi piksel basina isler.
namespace DynamicLights {
    constexpr int MAX_SHADER_LIGHTS = 8; // worldlit.frag'daki dizi boyutuyla ayni olmali

    struct Light {
        glm::vec3 position{ 0.0f };
        glm::vec3 color{ 1.0f };  // isik rengi * siddet
        float radius = 200.0f;
    };

    void Clear();
    void Add(const glm::vec3& position, const glm::vec3& color, float radius);

    // Sunucudan gelen entity'lerden isik uretir (su an yalniz mermiler: kirmizi).
    void CollectFromEntities(const std::vector<NetEntityState>& entities);

    // r_dlights cvar'i acik mi?
    bool IsEnabled();

    // Goz konumuna en yakin en fazla MAX_SHADER_LIGHTS isik.
    std::vector<Light> GetNearest(const glm::vec3& eye);
}