#include "FxCommon.h"
#include <cmath>
#include <random>
#include <windows.h>
#include <GL/gl.h>
#include "BSPMap.h"
#include "../ConVarRegistry.h"

namespace {

    constexpr int kMaxTicksPerFrame = 8;

    // Ground probe used to find the surface normal below an effect.
    constexpr float kProbeHeightAbove = 16.0f;
    constexpr float kProbeDepthBelow = 32.0f;
    constexpr float kMinWalkableNormalY = 0.3f;

    std::mt19937& RandomEngine() {
        static std::mt19937 engine{ std::random_device{}() };
        return engine;
    }
    int* g_fallbackShapesCvar = nullptr;   // "fx_fallback_shapes" degiskeninin degeri

}  // namespace

float Fx::Clamp(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

float Fx::Smoothstep(float edge0, float edge1, float x) {
    const float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Fx::RandomFloat(float low, float high) {
    if (high <= low) return low;
    return std::uniform_real_distribution<float>(low, high)(RandomEngine());
}

glm::vec3 Fx::RandomDirection() {
    // Rejection sampling: pick points in the unit cube until one falls inside the sphere.
    for (;;) {
        const glm::vec3 candidate(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f));
        const float lengthSquared = glm::dot(candidate, candidate);
        if (lengthSquared > 0.0001f && lengthSquared <= 1.0f) {
            return candidate / std::sqrt(lengthSquared);
        }
    }
}

glm::vec3 Fx::DirectionFromAngles(float yawDegrees, float pitchDegrees) {
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);
    return glm::vec3(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
}

void Fx::BuildPlaneBasis(const glm::vec3& normal, glm::vec3& outTangent, glm::vec3& outBitangent) {
    const bool nearlyVertical = std::fabs(normal.y) >= 0.9f;
    const glm::vec3 helper = nearlyVertical ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    outTangent = glm::normalize(glm::cross(normal, helper));
    outBitangent = glm::cross(normal, outTangent);
}

glm::vec3 Fx::GroundNormalAt(const glm::vec3& position) {
    const glm::vec3 from = position + glm::vec3(0.0f, kProbeHeightAbove, 0.0f);
    const glm::vec3 to = position - glm::vec3(0.0f, kProbeDepthBelow, 0.0f);
    const TraceResult trace = g_Map.TraceLine(from, to, 0);

    if (trace.fraction < 1.0f && glm::length(trace.planeNormal) > 0.5f) {
        const glm::vec3 normal = glm::normalize(trace.planeNormal);
        if (normal.y > kMinWalkableNormalY) return normal;
    }
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

int Fx::CountDueTicks(float& accumulator, float interval, float deltaTime) {
    accumulator += deltaTime;

    int dueCount = 0;
    while (accumulator >= interval && dueCount < kMaxTicksPerFrame) {
        accumulator -= interval;
        ++dueCount;
    }

    // After a long stall, drop the backlog instead of emitting a huge burst later.
    if (accumulator > interval) accumulator = 0.0f;
    return dueCount;
}

void Fx::ApplyDirectionTransform(float yawDegrees, float pitchDegrees) {
    glRotatef(-yawDegrees, 0.0f, 1.0f, 0.0f);
    glRotatef(pitchDegrees, 0.0f, 0.0f, 1.0f);
}

void Fx::EmitTexturedVertex(float u, float v, const glm::vec3& position) {
    glTexCoord2f(u, v);
    glVertex3f(position.x, position.y, position.z);
}

void Fx::RegisterCvars() {
    if (g_fallbackShapesCvar != nullptr) return;   // bir kez kaydedilir

    g_fallbackShapesCvar = ConVarRegistry::RegisterInt("fx_fallback_shapes", 1,
        "1: sprite dosyasi yoksa raygun isini icin dokusuz yedek sekil cizilir, 0: yalnizca dosyalar", CVAR_NONE);
}

bool Fx::FallbackShapesEnabled() {
    // Degisken henuz kaydedilmediyse varsayilan olarak acik kabul edilir.
    return g_fallbackShapesCvar == nullptr || *g_fallbackShapesCvar != 0;
}