#pragma once
#include"DynamicLight.h"
#include"BVH.h"
#include<vector>

class DynamicLightManager {
public:
    // Yeni bir dinamik isik ekler, referans/id doner (sonradan silmek/degistirmek icin gerekmiyor simdilik)
    void AddExplosion(const glm::vec3& position, float radius = 250.0f, glm::vec3 color = glm::vec3(1.0f, 0.6f, 0.2f));
    void AddTorch(const glm::vec3& position, float radius = 150.0f, glm::vec3 color = glm::vec3(1.0f, 0.8f, 0.5f));
    void AddMuzzleFlash(const glm::vec3& position, float radius = 100.0f, glm::vec3 color = glm::vec3(1.0f, 0.9f, 0.6f));

    // Her frame cagrilir: yaslandirma, sonme, olu isiklari temizleme
    void Update(float deltaTime);

    // BVH'den etkilenen primitive'leri bulup vertex renklerini gunceller.
    // Sadece isiklarin etki alanindaki brush'lar guncellenir (performans icin).
    void ApplyToWorld(BVH& worldBVH);

    const std::vector<DynamicLight>& GetLights() const { return m_lights; }

private:
    float ComputeCurrentIntensity(const DynamicLight& light) const;
    glm::vec3 ComputeContribution(const glm::vec3& worldPos, const glm::vec3& normal, const DynamicLight& light) const;

    std::vector<DynamicLight> m_lights;
};

extern DynamicLightManager g_DynamicLights; // Engine.h'deki üye yerine global kullanmak istersen