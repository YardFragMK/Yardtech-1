#pragma once
#include <glm/glm.hpp>

// Bir botun firlattigi enerji topu. direction, atildigi anda hedefe kilitlenir
// ve degismez -- top oyuncuyu takip etmez, sabit bir dogrultuda ilerler.
struct BotProjectile {
    glm::vec3 position{ 0.0f };
    glm::vec3 direction{ 0.0f, 0.0f, 1.0f };
    float speed = 300.0f;
    int damage = 15;
    bool alive = true;
};