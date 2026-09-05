#pragma once
#include <glm/glm.hpp>
#include <array>

// Basit AABB vs frustum culling icin 6 duzlemli frustum.
// Duzlem denklemi: normal . p + d = 0, normal ic tarafa (frustum'un icine) bakar.
struct FrustumPlane {
    glm::vec3 normal{ 0.0f };
    float d = 0.0f;

    float DistanceToPoint(const glm::vec3& p) const {
        return glm::dot(normal, p) + d;
    }
};

struct Frustum {
    // sira onemli degil, hepsi test ediliyor
    std::array<FrustumPlane, 6> planes;

    // vp: projection * view (ayni sirayla carpilmis kombinasyon matrisi)
    static Frustum FromViewProjection(const glm::mat4& vp);

    // AABB frustum'un tamamen disindaysa false, kesisiyor/icindeyse true
    bool IntersectsAABB(const glm::vec3& mins, const glm::vec3& maxs) const;
};