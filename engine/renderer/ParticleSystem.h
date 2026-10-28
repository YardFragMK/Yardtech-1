#pragma once
#include <glm/glm.hpp>
#include "SpriteSystem.h"

// CPU tabanli sprite parcaciklari (GoldSrc tarzi): kameraya donuk, omur boyunca renk ve
// boyut degistiren, ek (additive) ya da alfa karisimli dortgenler.
struct Particle {
    glm::vec3 pos{ 0.0f };
    glm::vec3 vel{ 0.0f };
    glm::vec3 accel{ 0.0f };   // yercekimi ya da yukselme
    float drag = 0.0f;         // 1/sn
    float age = 0.0f;
    float life = 1.0f;
    float size0 = 10.0f;       // omrun basindaki ve sonundaki boyut
    float size1 = 10.0f;
    float rot = 0.0f;          // radyan
    float rotVel = 0.0f;
    glm::vec4 color0{ 1.0f };  // omrun basindaki ve sonundaki renk (rgba)
    glm::vec4 color1{ 1.0f, 1.0f, 1.0f, 0.0f };
    SpriteId sprite = SPR_GLOW;
    bool additive = false;
    bool animate = false;      // omur boyunca sprite karelerini oynatir
    int frame = 0;             // animate kapaliysa sabit kare
    float fadeIn = 0.0f;       // omrun bu orani boyunca alfa 0'dan yukselir
    float nearFade = 0.0f;     // kameraya bu mesafeden yakin olunca solar
};

namespace ParticleSystem {
    void Spawn(const Particle& particle);
    void Update(float deltaTime);
    void Render(const glm::vec3& cameraPos, const glm::vec3& cameraRight, const glm::vec3& cameraUp);
    bool Empty();
    void Clear();
}