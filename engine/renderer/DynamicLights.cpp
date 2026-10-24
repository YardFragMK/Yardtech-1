#include "DynamicLights.h"
#include "../ConVarRegistry.h"
#include <algorithm>

namespace {
    constexpr size_t MAX_COLLECTED_LIGHTS = 64;

    std::vector<DynamicLights::Light> s_lights;

    int* s_enabled = nullptr;
    float* s_radius = nullptr;
    float* s_intensity = nullptr;

    // Cvar'lar ilk kullanimda kaydedilir; kayit tekrarlanirsa ayni isaretciyi dondurur.
    void EnsureCvars() {
        if (s_enabled != nullptr) return;

        s_enabled = ConVarRegistry::RegisterInt("r_dlights", 1, "Mermi isiklari: piksel basina dinamik isik (0/1)", CVAR_NONE);
        s_radius = ConVarRegistry::RegisterFloat("r_dlight_radius", 220.0f, "Mermi isigi yaricapi (birim)", CVAR_NONE);
        s_intensity = ConVarRegistry::RegisterFloat("r_dlight_intensity", 1.4f, "Mermi isigi siddeti", CVAR_NONE);
    }
}

void DynamicLights::Clear() {
    s_lights.clear();
}

void DynamicLights::Add(const glm::vec3& position, const glm::vec3& color, float radius) {
    if (s_lights.size() >= MAX_COLLECTED_LIGHTS) return;

    Light light;
    light.position = position;
    light.color = color;
    light.radius = radius;
    s_lights.push_back(light);
}

bool DynamicLights::IsEnabled() {
    EnsureCvars();
    return s_enabled != nullptr && *s_enabled != 0;
}

void DynamicLights::CollectFromEntities(const std::vector<NetEntityState>& entities) {
    EnsureCvars();
    if (!IsEnabled()) return;

    const float radius = (s_radius != nullptr) ? *s_radius : 220.0f;
    const float intensity = (s_intensity != nullptr) ? *s_intensity : 1.4f;

    for (const NetEntityState& entity : entities) {
        if (entity.classId != ENT_CLASS_PROJECTILE) continue;

        // Enerji topu: kirmizimsi turuncu isik.
        Add(entity.origin, glm::vec3(1.0f, 0.25f, 0.12f) * intensity, radius);
    }
}

std::vector<DynamicLights::Light> DynamicLights::GetNearest(const glm::vec3& eye) {
    std::vector<Light> sorted = s_lights;

    std::sort(sorted.begin(), sorted.end(), [&eye](const Light& a, const Light& b) {
        const glm::vec3 da = a.position - eye;
        const glm::vec3 db = b.position - eye;
        return glm::dot(da, da) < glm::dot(db, db);
        });

    if (sorted.size() > static_cast<size_t>(MAX_SHADER_LIGHTS)) sorted.resize(MAX_SHADER_LIGHTS);
    return sorted;
}