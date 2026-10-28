#include "FxEmitters.h"
#include "FxTuning.h"

namespace {

    namespace Blast = FxTuning::Explosion;
    namespace Bfg = FxTuning::BfgExplosion;

    //--------------------------------------------------------------------------
    // Regular explosion
    //--------------------------------------------------------------------------
    void SpawnBlastFireballs(const glm::vec3& center, float radius) {
        Fx::ParticleBuilder(SPR_FIRE)
            .At(center)
            .Life(Blast::kMainFireballLife)
            .Size(radius * Blast::kMainFireballStartScale, radius * Blast::kMainFireballEndScale)
            .Color(Blast::kMainFireballColor)
            .Additive()
            .Animated()
            .RandomRotation()
            .Spawn();

        for (int i = 0; i < Blast::kSmallFireballCount; ++i) {
            Fx::ParticleBuilder(SPR_FIRE)
                .At(center + Fx::RandomDirection() * (radius * Blast::kSmallFireballOffsetScale))
                .Velocity(Fx::RandomDirection() * (radius * Blast::kSmallFireballSpeedScale))
                .Life(Blast::kSmallFireballLife.Random())
                .Size(radius * Blast::kSmallFireballStartScale, radius * Blast::kSmallFireballEndScale)
                .Color(Blast::kSmallFireballColor)
                .Additive()
                .Animated()
                .RandomRotation()
                .Spawn();
        }
    }

    void SpawnBlastFlashAndShockwave(const glm::vec3& center, float radius) {
        Fx::ParticleBuilder(SPR_GLOW)
            .At(center)
            .Life(Blast::kCoreGlowLife)
            .Size(radius * Blast::kCoreGlowStartScale, radius * Blast::kCoreGlowEndScale)
            .Color(Blast::kCoreGlowColor)
            .Additive()
            .Spawn();

        Fx::ParticleBuilder(SPR_RING)
            .At(center)
            .Life(Blast::kShockwaveLife)
            .Size(radius * Blast::kShockwaveStartScale, radius * Blast::kShockwaveEndScale)
            .Color(Blast::kShockwaveColor)
            .Additive()
            .Spawn();
    }

    void SpawnBlastSparks(const glm::vec3& center, float radius) {
        const int count = static_cast<int>(Fx::Clamp(
            Blast::kMinSparks + radius / Blast::kRadiusPerExtraSpark, Blast::kMinSparks, Blast::kMaxSparks));
        const float speedScale = radius / Blast::kSparkReferenceRadius;

        for (int i = 0; i < count; ++i) {
            Fx::ParticleBuilder(SPR_SPARK)
                .At(center)
                .Velocity(Fx::RandomDirection() * (Blast::kSparkSpeed.Random() * speedScale))
                .Life(Blast::kSparkLife.Random())
                .Size(Blast::kSparkStartSize, Blast::kSparkEndSize)
                .Color(Blast::kSparkColor)
                .Gravity(Blast::kSparkGravity)
                .Drag(Blast::kSparkDrag)
                .Additive()
                .Spawn();
        }
    }

    void SpawnBlastSmoke(const glm::vec3& center, float radius) {
        const int count = static_cast<int>(Fx::Clamp(
            Blast::kMinSmoke + radius / Blast::kRadiusPerExtraSmoke, Blast::kMinSmoke, Blast::kMaxSmoke));

        for (int i = 0; i < count; ++i) {
            const glm::vec3 drift = Fx::RandomDirection() * Blast::kSmokeDrift.Random();
            const glm::vec3 rise(0.0f, Blast::kSmokeRise.Random(), 0.0f);

            Fx::ParticleBuilder(SPR_SMOKE)
                .At(center + Fx::RandomDirection() * (radius * Blast::kSmokeOffsetScale))
                .Velocity(drift + rise)
                .Life(Blast::kSmokeLife.Random())
                .Size(radius * Blast::kSmokeStartScale, radius * Blast::kSmokeEndScale)
                .Color(Blast::kSmokeColor)
                .Drag(Blast::kSmokeDrag)
                .RandomRotation(Blast::kSmokeSpin)
                .FadeIn(Blast::kSmokeFadeIn)
                .FadeNearCamera(Blast::kSmokeNearFade)
                .Spawn();
        }
    }

    //--------------------------------------------------------------------------
    // BFG (lastgun) explosion: green, bigger, longer
    //--------------------------------------------------------------------------
    void SpawnBfgFireballs(const glm::vec3& center, float radius) {
        for (int i = 0; i < Bfg::kFireballCount; ++i) {
            const bool isCenter = (i == 0);
            const float scale = isCenter ? Bfg::kCenterFireballScale : Bfg::kSideFireballScale;
            const glm::vec3 position = isCenter
                ? center
                : center + Fx::RandomDirection() * (radius * Bfg::kSideFireballOffsetScale);

            Fx::ParticleBuilder(SPR_FIRE)
                .At(position)
                .Life(Bfg::kFireballLife.Random())
                .Size(radius * Bfg::kFireballStartScale * scale, radius * Bfg::kFireballEndScale * scale)
                .Color(Bfg::kFireballColor)
                .Additive()
                .Animated()
                .RandomRotation()
                .Spawn();
        }
    }

    void SpawnBfgGlowAndShockwaves(const glm::vec3& center, float radius) {
        Fx::ParticleBuilder(SPR_GLOW)
            .At(center)
            .Life(Bfg::kGlowLife)
            .Size(radius * Bfg::kGlowStartScale, radius * Bfg::kGlowEndScale)
            .Color(Bfg::kGlowColor)
            .Additive()
            .Spawn();

        Fx::ParticleBuilder(SPR_RING)
            .At(center)
            .Life(Bfg::kInnerRingLife)
            .Size(radius * Bfg::kInnerRingStartScale, radius * Bfg::kInnerRingEndScale)
            .Color(Bfg::kInnerRingColor)
            .Additive()
            .Spawn();

        Fx::ParticleBuilder(SPR_RING)
            .At(center)
            .Life(Bfg::kOuterRingLife)
            .Size(radius * Bfg::kOuterRingStartScale, radius * Bfg::kOuterRingEndScale)
            .Color(Bfg::kOuterRingColor)
            .Additive()
            .Spawn();
    }

    void SpawnBfgSparks(const glm::vec3& center) {
        for (int i = 0; i < Bfg::kSparkCount; ++i) {
            Fx::ParticleBuilder(SPR_SPARK)
                .At(center)
                .Velocity(Fx::RandomDirection() * Bfg::kSparkSpeed.Random())
                .Life(Bfg::kSparkLife.Random())
                .Size(Bfg::kSparkStartSize, Bfg::kSparkEndSize)
                .Color(Bfg::kSparkColor)
                .Gravity(Bfg::kSparkGravity)
                .Drag(Bfg::kSparkDrag)
                .Additive()
                .Spawn();
        }
    }

    void SpawnBfgSmoke(const glm::vec3& center, float radius) {
        for (int i = 0; i < Bfg::kSmokeCount; ++i) {
            const glm::vec3 drift = Fx::RandomDirection() * Bfg::kSmokeDrift.Random();
            const glm::vec3 rise(0.0f, Bfg::kSmokeRise.Random(), 0.0f);

            Fx::ParticleBuilder(SPR_SMOKE)
                .At(center + Fx::RandomDirection() * (radius * Bfg::kSmokeOffsetScale))
                .Velocity(drift + rise)
                .Life(Bfg::kSmokeLife.Random())
                .Size(radius * Bfg::kSmokeStartScale, radius * Bfg::kSmokeEndScale)
                .Color(Bfg::kSmokeColor)
                .Drag(Bfg::kSmokeDrag)
                .RandomRotation(Bfg::kSmokeSpin)
                .FadeIn(Bfg::kSmokeFadeIn)
                .FadeNearCamera(Bfg::kSmokeNearFade)
                .Spawn();
        }
    }

}  // namespace

void Fx::SpawnExplosion(const glm::vec3& center, float radius) {
    SpawnBlastFireballs(center, radius);
    SpawnBlastFlashAndShockwave(center, radius);
    SpawnBlastSparks(center, radius);
    SpawnBlastSmoke(center, radius);
}

void Fx::SpawnBfgExplosion(const glm::vec3& center, float radius) {
    SpawnBfgFireballs(center, radius);
    SpawnBfgGlowAndShockwaves(center, radius);
    SpawnBfgSparks(center);
    SpawnBfgSmoke(center, radius);
}