#pragma once
#include "FxCommon.h"

// Entry points of the individual effects. The director (WeaponEffects.cpp) decides
// WHEN to call them; each Fx*.cpp file decides what the effect looks like.
namespace Fx {

    //--- Particle emitters -------------------------------------------------------
    void SpawnExplosion(const glm::vec3& center, float radius);
    void SpawnBfgExplosion(const glm::vec3& center, float radius);
    void SpawnRayBeamFlashes(const NetEntityState& beam);

    void EmitFog(const NetEntityState& cloud, EmitterState& state, float deltaTime, bool justAppeared);
    void EmitPool(const NetEntityState& pool, EmitterState& state, float deltaTime, bool justAppeared, bool isAcid);
    void EmitProjectileTrail(const NetEntityState& projectile, EmitterState& state, float deltaTime);

    //--- Geometry drawn every frame (translucent pass, depth writes off) ---------
    void DrawEnergyOrb(const glm::vec3& center, float size, const glm::vec3& color, float seed, float spikes,
        float timeSeconds, const CameraBasis& camera);
    void DrawRayBeam(const NetEntityState& beam, const CameraBasis& camera);
    void DrawLiquidPool(const NetEntityState& pool, EmitterState& state, bool isAcid, float timeSeconds);

}  // namespace Fx