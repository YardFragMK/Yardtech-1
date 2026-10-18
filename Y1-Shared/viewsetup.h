#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Client'in her frame engine'e verdigi kamera bilgisi. Engine projeksiyon ve
// view matrisini bundan kurar; kamera mantiginin sahibi client.dll'dir.
struct ViewSetup {
    glm::vec3 origin{ 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float fovY = 75.0f;
    float zNear = 1.0f;
    float zFar = 8192.0f;
};

inline glm::vec3 ViewForward(const ViewSetup& view) {
    const float yawRad = glm::radians(view.yaw);
    const float pitchRad = glm::radians(view.pitch);
    return glm::normalize(glm::vec3(
        std::cos(yawRad) * std::cos(pitchRad),
        std::sin(pitchRad),
        std::sin(yawRad) * std::cos(pitchRad)));
}

// Eski Camera::GetViewMatrix ile ayni sonucu (roll dahil, cift hassasiyetli)
// uretir. Renderer ve baska engine kodlari kamera sinifina bagimli olmadan
// bunu kullanir.
inline glm::dmat4 BuildViewMatrix(const ViewSetup& view) {
    const glm::vec3 forward = ViewForward(view);
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    const glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(view.roll), forward);
    up = glm::normalize(glm::vec3(rollMatrix * glm::vec4(up, 0.0f)));

    const glm::dvec3 eye(view.origin);
    return glm::lookAt(eye, eye + glm::dvec3(forward), glm::dvec3(up));
}