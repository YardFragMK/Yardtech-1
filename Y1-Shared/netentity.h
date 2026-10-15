#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// Server'in client'a bildirdigi entity ozeti (replikasyon). classId oyun/mod
// tarafindan tanimlanir; engine bu degeri yorumlamaz.
struct NetEntityState {
    int32_t id = 0;
    uint16_t classId = 0;
    uint16_t flags = 0;
    int16_t health = 0;
    glm::vec3 origin{ 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
};