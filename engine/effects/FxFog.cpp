#include "FxEmitters.h"
#include "FxTuning.h"
#include <cmath>

namespace {

    namespace Tune = FxTuning::Fog;

    enum TimerSlot { kSteadyPuffTimer = 0 };

    // One drifting smoke puff somewhere inside the cloud.
    void SpawnPuff(const glm::vec3& position, const glm::vec3& velocity, float life, float cloudRadius) {
        Fx::ParticleBuilder(SPR_SMOKE)
            .At(position)
            .Velocity(velocity)
            .Life(life)
            .Size(cloudRadius * Tune::kPuffStartScale, cloudRadius * Tune::kPuffEndScale)
            .Color(Tune::kPuffColor)
            .Drag(Tune::kPuffDrag)
            .RandomRotation(Tune::kPuffSpin)
            .FadeIn(Tune::kPuffFadeIn)
            .FadeNearCamera(Tune::kPuffNearFade)
            .Spawn();
    }

    // When the cloud appears it bursts outward from the centre, so it fills quickly.
    void SpawnInitialBurst(const glm::vec3& center, float radius) {
        for (int i = 0; i < Tune::kBurstPuffCount; ++i) {
            const glm::vec3 direction = Fx::RandomDirection();
            const float distance = radius * Tune::kBurstSpreadScale * Fx::RandomFloat(0.0f, 1.0f);

            SpawnPuff(center + direction * distance,
                direction * Tune::kBurstSpeed.Random(),
                Tune::kBurstPuffLife.Random(),
                radius);
        }
    }

    // While the cloud lives, new puffs keep appearing at random points of its volume.
    void SpawnSteadyPuffs(const glm::vec3& center, float radius, Fx::EmitterState& state, float deltaTime) {
        const int dueCount = Fx::CountDueTicks(state.timers[kSteadyPuffTimer], Tune::kSteadyPuffInterval, deltaTime);

        for (int i = 0; i < dueCount; ++i) {
            // The cube root makes points uniform over the sphere's volume (not just its radius).
            const float distance = radius * Tune::kSteadySpreadScale * std::cbrt(Fx::RandomFloat(0.0f, 1.0f));
            const glm::vec3 position = center + Fx::RandomDirection() * distance;

            SpawnPuff(position,
                Fx::RandomDirection() * Tune::kSteadyDrift.Random(),
                Tune::kSteadyPuffLife.Random(),
                radius);
        }
    }

}  // namespace

void Fx::EmitFog(const NetEntityState& cloud, EmitterState& state, float deltaTime, bool justAppeared) {
    const float radius = RadiusOf(cloud);
    const float age = AgeSeconds(cloud);
    const float lifetime = cloud.pitch;

    if (justAppeared && age < Tune::kBurstWindow) {
        SpawnInitialBurst(cloud.origin, radius);
    }

    const bool nearEnd = age >= lifetime - Tune::kStopRefillBeforeEnd;
    if (!nearEnd) {
        SpawnSteadyPuffs(cloud.origin, radius, state, deltaTime);
    }
}