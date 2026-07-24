#pragma once
#include"BVH.h"
#include<glm/glm.hpp>

inline Frustum ExtractFrustum(const glm::mat4& viewProj) {
    Frustum f;
    glm::mat4 m = glm::transpose(viewProj); // satir bazli erisim icin transpoze

    // Left, Right, Bottom, Top, Near, Far
    f.planes[0].normal = glm::vec3(m[3] + m[0]); f.planes[0].dist = m[3][3] + m[0][3]; // left
    f.planes[1].normal = glm::vec3(m[3] - m[0]); f.planes[1].dist = m[3][3] - m[0][3]; // right
    f.planes[2].normal = glm::vec3(m[3] + m[1]); f.planes[2].dist = m[3][3] + m[1][3]; // bottom
    f.planes[3].normal = glm::vec3(m[3] - m[1]); f.planes[3].dist = m[3][3] - m[1][3]; // top
    f.planes[4].normal = glm::vec3(m[3] + m[2]); f.planes[4].dist = m[3][3] + m[2][3]; // near
    f.planes[5].normal = glm::vec3(m[3] - m[2]); f.planes[5].dist = m[3][3] - m[2][3]; // far

    for (auto& p : f.planes) {
        float len = glm::length(p.normal);
        if (len > 1e-6f) {
            p.normal /= len;
            p.dist /= len;
        }
    }

    return f;
}