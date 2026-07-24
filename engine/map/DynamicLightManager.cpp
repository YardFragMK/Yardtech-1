#include"DynamicLightManager.h"
#include<algorithm>
#include<cmath>
#include<cstdlib>

void DynamicLightManager::AddExplosion(const glm::vec3& position, float radius, glm::vec3 color) {
    DynamicLight light;
    light.position = position;
    light.radius = radius;
    light.color = color;
    light.type = DynamicLightType::Explosion;
    light.intensity = 1.0f;
    light.lifetime = 0.6f; // yarim saniyeden biraz fazla surer, hizla soner
    light.age = 0.0f;
    m_lights.push_back(light);
}

void DynamicLightManager::AddTorch(const glm::vec3& position, float radius, glm::vec3 color) {
    DynamicLight light;
    light.position = position;
    light.radius = radius;
    light.color = color;
    light.type = DynamicLightType::Torch;
    light.intensity = 1.0f;
    light.lifetime = 0.0f; // sonsuz, elle silinmedikce kalir
    light.flickerSeed = (float)(rand() % 1000) / 1000.0f * 6.28f; // rastgele faz, hepsi ayni anda titremesin
    m_lights.push_back(light);
}

void DynamicLightManager::AddMuzzleFlash(const glm::vec3& position, float radius, glm::vec3 color) {
    DynamicLight light;
    light.position = position;
    light.radius = radius;
    light.color = color;
    light.type = DynamicLightType::MuzzleFlash;
    light.intensity = 1.0f;
    light.lifetime = 0.08f; // cok kisa, tek-iki kare
    m_lights.push_back(light);
}

void DynamicLightManager::Update(float deltaTime) {
    for (auto& light : m_lights) {
        light.age += deltaTime;
        if (light.lifetime > 0.0f && light.age >= light.lifetime) {
            light.markedForRemoval = true;
        }
    }

    m_lights.erase(
        std::remove_if(m_lights.begin(), m_lights.end(),
            [](const DynamicLight& l) { return l.markedForRemoval; }),
        m_lights.end()
    );
}

float DynamicLightManager::ComputeCurrentIntensity(const DynamicLight& light) const {
    switch (light.type) {
    case DynamicLightType::Explosion: {
        // Hizla 1.0'dan 0.0'a soner (ust-konveks azalma, patlama hissi icin)
        float t = (light.lifetime > 0.0f) ? (light.age / light.lifetime) : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        return (1.0f - t) * (1.0f - t); // kareli azalma: basta yavas, sonda hizli soner
    }
    case DynamicLightType::MuzzleFlash: {
        float t = (light.lifetime > 0.0f) ? (light.age / light.lifetime) : 0.0f;
        return (t < 1.0f) ? 1.0f : 0.0f; // sabit parlaklik, bitince aniden sil
    }
    case DynamicLightType::Torch: {
        // Hafif rastgele titreme: birden fazla sinus dalgasinin toplami daha organik durur
        float t = light.age + light.flickerSeed;
        float flicker = 0.85f
            + 0.10f * sinf(t * 9.0f)
            + 0.05f * sinf(t * 23.0f);
        return std::clamp(flicker, 0.0f, 1.0f);
    }
    }
    return 1.0f;
}

glm::vec3 DynamicLightManager::ComputeContribution(const glm::vec3& worldPos, const glm::vec3& normal, const DynamicLight& light) const {
    glm::vec3 toLight = light.position - worldPos;
    float dist = glm::length(toLight);
    if (dist > light.radius || dist < 0.001f) {
        return glm::vec3(0.0f); // menzil disi
    }

    glm::vec3 lightDir = toLight / dist;
    float ndotl = std::max(glm::dot(normal, lightDir), 0.0f);
    if (ndotl <= 0.0f) return glm::vec3(0.0f);

    // Menzilin sonuna dogru yumusakca sonen bir azalma (linear falloff, radius disinda 0)
    float falloff = 1.0f - (dist / light.radius);
    falloff = falloff * falloff; // biraz daha yumusak gecis

    float currentIntensity = ComputeCurrentIntensity(light);

    return light.color * ndotl * falloff * currentIntensity * light.intensity;
}

void DynamicLightManager::ApplyToWorld(BVH& worldBVH) {
    if (m_lights.empty()) {
        // Dinamik isik yoksa, tum vertex'leri sadece base color'a geri dondur
        // (onceki frame'de eklenmis bir isigin kalintisini temizlemek icin)
        for (auto& prim : const_cast<std::vector<BVHPrimitive>&>(worldBVH.GetPrimitives())) {
            for (auto& face : prim.mesh.faces) {
                for (auto& v : face.vertices) {
                    v.color = v.baseColor;
                }
            }
        }
        return;
    }

    // Her isik icin, o isigin menzilindeki primitive'leri BVH'den sorgula
    // ve SADECE onlarin vertex'lerini guncelle (performans optimizasyonu).
    // Once tum world'u base color'a resetle (onceki frame'in kalintilarini temizle).
    auto& primitives = const_cast<std::vector<BVHPrimitive>&>(worldBVH.GetPrimitives());
    for (auto& prim : primitives) {
        for (auto& face : prim.mesh.faces) {
            for (auto& v : face.vertices) {
                v.color = v.baseColor;
            }
        }
    }

    for (const auto& light : m_lights) {
        BVHBounds queryBounds;
        queryBounds.min = light.position - glm::vec3(light.radius);
        queryBounds.max = light.position + glm::vec3(light.radius);

        std::vector<int> nearbyIndices;
        worldBVH.QueryBounds(queryBounds, nearbyIndices);

        for (int idx : nearbyIndices) {
            auto& prim = primitives[idx];
            for (auto& face : prim.mesh.faces) {
                for (auto& v : face.vertices) {
                    glm::vec3 contribution = ComputeContribution(v.position, v.normal, light);
                    v.color = glm::clamp(v.color + contribution, glm::vec3(0.0f), glm::vec3(1.0f));
                }
            }
        }
    }
}