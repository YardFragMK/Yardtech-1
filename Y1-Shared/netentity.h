#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// Entity sinif kimlikleri. Engine bu degerleri yorumlamaz; oyun (server.dll) ve
// onu cizen taraf (client/exe) arasinda ortak sozlesmedir.
enum EntityClassId : uint16_t {
    ENT_CLASS_NONE = 0,
    ENT_CLASS_BOT_STILAR = 1,
    ENT_CLASS_PROJECTILE = 2
};

// Server'in client'a bildirdigi entity ozeti (replikasyon).
struct NetEntityState {
    int32_t id = 0;
    uint16_t classId = ENT_CLASS_NONE;
    uint16_t flags = 0;
    int16_t health = 0;
    glm::vec3 origin{ 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
};