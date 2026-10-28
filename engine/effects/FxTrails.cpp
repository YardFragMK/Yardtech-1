#include "FxEmitters.h"
#include "FxTuning.h"

namespace {

    namespace Trail = FxTuning::Trail;

    // Names for the emission timers of each trail type.
    enum RocketTimer { kRocketSmokeTimer = 0, kRocketFlameTimer = 1 };
    enum LastBallTimer { kLastBallSparkTimer = 0, kLastBallGlowTimer = 1 };
    enum SingleTimer { kTrailTimer = 0 };

    //--------------------------------------------------------------------------
    void EmitRocketTrail(const NetEntityState& rocket, Fx::EmitterState& state, float deltaTime) {
        namespace Tune = Trail::Rocket;
        const glm::vec3 direction = Fx::DirectionFromAngles(rocket.yaw, rocket.pitch);

        const int smokeCount = Fx::CountDueTicks(state.timers[kRocketSmokeTimer], Tune::kSmokeInterval, deltaTime);
        for (int i = 0; i < smokeCount; ++i) {
            const glm::vec3 position = rocket.origin - direction * Tune::kSmokeBackOffset
                + Fx::RandomDirection() * Tune::kSmokeJitter;
            const glm::vec3 velocity = -direction * Tune::kSmokeBackSpeed
                + Fx::RandomDirection() * Tune::kSmokeScatterSpeed;

            Fx::ParticleBuilder(SPR_SMOKE)
                .At(position)
                .Velocity(velocity)
                .Life(Tune::kSmokeLife.Random())
                .Size(Tune::kSmokeStartSize, Tune::kSmokeEndSize)
                .Color(Tune::kSmokeColor)
                .RandomRotation()
                .FadeIn(Tune::kSmokeFadeIn)
                .FadeNearCamera(Tune::kSmokeNearFade)
                .Spawn();
        }

        const int flameCount = Fx::CountDueTicks(state.timers[kRocketFlameTimer], Tune::kFlameInterval, deltaTime);
        for (int i = 0; i < flameCount; ++i) {
            Fx::ParticleBuilder(SPR_GLOW)
                .At(rocket.origin - direction * Tune::kFlameBackOffset)
                .Life(Tune::kFlameLife)
                .Size(Tune::kFlameStartSize, Tune::kFlameEndSize)
                .Color(Tune::kFlameColor)
                .Additive()
                .Spawn();
        }
    }

    //--------------------------------------------------------------------------
    void EmitEnergyBallTrail(const NetEntityState& ball, Fx::EmitterState& state, float deltaTime,
        const Trail::EnergyBall::Style& style) {
        namespace Tune = Trail::EnergyBall;

        const Fx::ColorRamp color = {
            glm::vec4(style.color, 1.0f),
            glm::vec4(style.color * Tune::kFadedColorScale, 0.0f)
        };

        const int count = Fx::CountDueTicks(state.timers[kTrailTimer], style.interval, deltaTime);
        for (int i = 0; i < count; ++i) {
            Fx::ParticleBuilder(SPR_SPARK)
                .At(ball.origin + Fx::RandomDirection() * Tune::kJitter)
                .Velocity(Fx::RandomDirection() * Tune::kScatterSpeed)
                .Life(Tune::kLife.Random())
                .Size(style.startSize, Tune::kEndSize)
                .Color(color)
                .Additive()
                .Spawn();
        }
    }

    //--------------------------------------------------------------------------
    void EmitLastBallTrail(const NetEntityState& ball, Fx::EmitterState& state, float deltaTime) {
        namespace Tune = Trail::LastBall;

        const int sparkCount = Fx::CountDueTicks(state.timers[kLastBallSparkTimer], Tune::kSparkInterval, deltaTime);
        for (int i = 0; i < sparkCount; ++i) {
            Fx::ParticleBuilder(SPR_SPARK)
                .At(ball.origin + Fx::RandomDirection() * Tune::kSparkJitter)
                .Velocity(Fx::RandomDirection() * Tune::kSparkScatterSpeed)
                .Life(Tune::kSparkLife.Random())
                .Size(Tune::kSparkStartSize, Tune::kSparkEndSize)
                .Color(Tune::kSparkColor)
                .Additive()
                .Spawn();
        }

        const int glowCount = Fx::CountDueTicks(state.timers[kLastBallGlowTimer], Tune::kGlowInterval, deltaTime);
        for (int i = 0; i < glowCount; ++i) {
            Fx::ParticleBuilder(SPR_GLOW)
                .At(ball.origin)
                .Life(Tune::kGlowLife)
                .Size(Tune::kGlowStartSize, Tune::kGlowEndSize)
                .Color(Tune::kGlowColor)
                .Additive()
                .Spawn();
        }
    }

    //--------------------------------------------------------------------------
    void EmitBombTrail(const NetEntityState& bomb, Fx::EmitterState& state, float deltaTime, bool isAcid) {
        namespace Tune = Trail::Bomb;

        const glm::vec3 hot = isAcid ? Tune::kAcidColor : Tune::kLavaColor;
        const Fx::ColorRamp color = {
            glm::vec4(hot, 1.0f),
            glm::vec4(hot * Tune::kFadedColorScale, 0.0f)
        };

        const int count = Fx::CountDueTicks(state.timers[kTrailTimer], Tune::kInterval, deltaTime);
        for (int i = 0; i < count; ++i) {
            Fx::ParticleBuilder(SPR_SPARK)
                .At(bomb.origin + Fx::RandomDirection() * Tune::kJitter)
                .Velocity(Fx::RandomDirection() * Tune::kScatterSpeed)
                .Life(Tune::kLife.Random())
                .Size(Tune::kStartSize, Tune::kEndSize)
                .Color(color)
                .Gravity(Tune::kGravity)
                .Additive()
                .Spawn();
        }
    }

    //--------------------------------------------------------------------------
    void EmitSawbladeTrail(const NetEntityState& blade, Fx::EmitterState& state, float deltaTime) {
        namespace Tune = Trail::Sawblade;

        const int count = Fx::CountDueTicks(state.timers[kTrailTimer], Tune::kInterval, deltaTime);
        for (int i = 0; i < count; ++i) {
            Fx::ParticleBuilder(SPR_SPARK)
                .At(blade.origin + Fx::RandomDirection() * Tune::kJitter)
                .Velocity(Fx::RandomDirection() * Tune::kScatterSpeed)
                .Life(Tune::kLife.Random())
                .Size(Tune::kStartSize, Tune::kEndSize)
                .Color(Tune::kColor)
                .Gravity(Tune::kGravity)
                .Additive()
                .Spawn();
        }
    }

}  // namespace

void Fx::EmitProjectileTrail(const NetEntityState& projectile, EmitterState& state, float deltaTime) {
    switch (projectile.classId) {
    case ENT_CLASS_ROCKET:
        EmitRocketTrail(projectile, state, deltaTime);
        break;
    case ENT_CLASS_PROJECTILE:
        EmitEnergyBallTrail(projectile, state, deltaTime, Trail::EnergyBall::kBot);
        break;
    case ENT_CLASS_STAPLE_BALL:
        EmitEnergyBallTrail(projectile, state, deltaTime, Trail::EnergyBall::kStaple);
        break;
    case ENT_CLASS_LAST_BALL:
        EmitLastBallTrail(projectile, state, deltaTime);
        break;
    case ENT_CLASS_LAVA_BOMB:
        EmitBombTrail(projectile, state, deltaTime, false);
        break;
    case ENT_CLASS_ACID_BOMB:
        EmitBombTrail(projectile, state, deltaTime, true);
        break;
    case ENT_CLASS_SAWBLADE:
        EmitSawbladeTrail(projectile, state, deltaTime);
        break;
    default:
        break;
    }
}