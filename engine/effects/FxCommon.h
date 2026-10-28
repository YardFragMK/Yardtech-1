#pragma once
#include <glm/glm.hpp>
#include "netentity.h"
#include "../renderer/ParticleSystem.h"

// Shared building blocks of the weapon visual effects: math helpers, a fluent
// particle builder and the per-entity emitter state.
namespace Fx {

    constexpr float kPi = 3.14159265f;

    //--------------------------------------------------------------------------
    // Math and randomness
    //--------------------------------------------------------------------------
    float Clamp(float value, float low, float high);
    float Smoothstep(float edge0, float edge1, float x);
    float RandomFloat(float low, float high);
    glm::vec3 RandomDirection();
    glm::vec3 DirectionFromAngles(float yawDegrees, float pitchDegrees);
    void BuildPlaneBasis(const glm::vec3& normal, glm::vec3& outTangent, glm::vec3& outBitangent);
    glm::vec3 GroundNormalAt(const glm::vec3& position);

    //--------------------------------------------------------------------------
    // Ayarlar
    //--------------------------------------------------------------------------
    // Efektlerin ortak konsol degiskenlerini kaydeder ("fx_fallback_shapes").
    void RegisterCvars();

    // true ise sprite dosyasi gerektiren bazi efektler, dosya yoksa dokusuz minimal bir sekil
    // cizer (raygun isin seridi ve sarmal noktalari). "fx_fallback_shapes" ile kontrol edilir:
    // 1 = acik (varsayilan), 0 = yalnizca dosyadan yuklenenler cizilir.
    bool FallbackShapesEnabled();

    // A closed range of random values.
    struct Span {
        float low;
        float high;

        float Random() const { return RandomFloat(low, high); }
    };

    // Colour at the start and at the end of a particle's life.
    struct ColorRamp {
        glm::vec4 start;
        glm::vec4 end;
    };

    //--------------------------------------------------------------------------
    // Time-based emission
    //--------------------------------------------------------------------------
    // Adds deltaTime to the accumulator and returns how many emissions are due
    // (one per interval, capped per frame). The remainder stays in the accumulator.
    int CountDueTicks(float& accumulator, float interval, float deltaTime);

    //--------------------------------------------------------------------------
    // Entities
    //--------------------------------------------------------------------------
    // Replicated entities carry their age in centiseconds (health) and their size
    // in world units (flags); see netentity.h.
    inline float AgeSeconds(const NetEntityState& entity) { return static_cast<float>(entity.health) / 100.0f; }
    inline float RadiusOf(const NetEntityState& entity) { return static_cast<float>(entity.flags); }

    // State an effect keeps between frames for one entity.
    struct EmitterState {
        float timers[3] = { 0.0f, 0.0f, 0.0f };   // emission accumulators; the meaning is per effect
        glm::vec3 surfaceNormal{ 0.0f, 1.0f, 0.0f };
        bool surfaceNormalKnown = false;
    };

    //--------------------------------------------------------------------------
    // OpenGL helpers
    //--------------------------------------------------------------------------
    struct CameraBasis {
        glm::vec3 position{ 0.0f };
        glm::vec3 right{ 1.0f, 0.0f, 0.0f };
        glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    };

    // Rotates the local +X axis to the given flight direction.
    void ApplyDirectionTransform(float yawDegrees, float pitchDegrees);

    // One textured vertex (call between glBegin and glEnd).
    void EmitTexturedVertex(float u, float v, const glm::vec3& position);

    //--------------------------------------------------------------------------
    // Fluent particle construction:
    //   ParticleBuilder(SPR_SMOKE).At(p).Life(2.0f).Size(10, 40).Color(ramp).Spawn();
    //--------------------------------------------------------------------------
    class ParticleBuilder {
    public:
        explicit ParticleBuilder(SpriteId sprite) { particle_.sprite = sprite; }

        ParticleBuilder& At(const glm::vec3& position) { particle_.pos = position; return *this; }
        ParticleBuilder& Velocity(const glm::vec3& velocity) { particle_.vel = velocity; return *this; }
        ParticleBuilder& Acceleration(const glm::vec3& accel) { particle_.accel = accel; return *this; }
        ParticleBuilder& Gravity(float strength) { particle_.accel = glm::vec3(0.0f, -strength, 0.0f); return *this; }
        ParticleBuilder& Drag(float perSecond) { particle_.drag = perSecond; return *this; }
        ParticleBuilder& Life(float seconds) { particle_.life = seconds; return *this; }
        ParticleBuilder& Size(float start, float end) { particle_.size0 = start; particle_.size1 = end; return *this; }
        ParticleBuilder& Color(const ColorRamp& ramp) { particle_.color0 = ramp.start; particle_.color1 = ramp.end; return *this; }
        ParticleBuilder& Additive() { particle_.additive = true; return *this; }
        ParticleBuilder& Animated() { particle_.animate = true; return *this; }
        ParticleBuilder& FadeIn(float lifeFraction) { particle_.fadeIn = lifeFraction; return *this; }
        ParticleBuilder& FadeNearCamera(float distance) { particle_.nearFade = distance; return *this; }

        // Random starting angle and (optionally) a random spin of up to maxSpinRate rad/s.
        ParticleBuilder& RandomRotation(float maxSpinRate = 0.0f) {
            particle_.rot = RandomFloat(0.0f, 2.0f * kPi);
            particle_.rotVel = RandomFloat(-maxSpinRate, maxSpinRate);
            return *this;
        }

        void Spawn() const { ParticleSystem::Spawn(particle_); }

    private:
        Particle particle_;
    };

}  // namespace Fx