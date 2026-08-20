#include "Frustum.h"

Frustum Frustum::FromViewProjection(const glm::mat4& m) {
    Frustum f;

    // Gribb-Hartmann yontemi. glm mat4 sutun-major oldugu icin m[col][row] ile erisiliyor.
    glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
    glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
    glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
    glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

    glm::vec4 raw[6] = {
        row3 + row0, // left
        row3 - row0, // right
        row3 + row1, // bottom
        row3 - row1, // top
        row3 + row2, // near
        row3 - row2  // far
    };

    for (int i = 0; i < 6; i++) {
        glm::vec3 n(raw[i].x, raw[i].y, raw[i].z);
        float len = glm::length(n);
        if (len < 1e-8f) len = 1e-8f;
        f.planes[i].normal = n / len;
        f.planes[i].d = raw[i].w / len;
    }

    return f;
}

bool Frustum::IntersectsAABB(const glm::vec3& mins, const glm::vec3& maxs) const {
    for (const auto& plane : planes) {
        // AABB'nin bu duzlem normaline gore "en ileri" kosesini bul (positive vertex)
        glm::vec3 p;
        p.x = (plane.normal.x >= 0.0f) ? maxs.x : mins.x;
        p.y = (plane.normal.y >= 0.0f) ? maxs.y : mins.y;
        p.z = (plane.normal.z >= 0.0f) ? maxs.z : mins.z;

        if (plane.DistanceToPoint(p) < 0.0f) {
            return false; // AABB bu duzlemin tamamen disinda -> frustum disinda
        }
    }
    return true;
}