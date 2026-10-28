#include "WeaponEffects.h"
#include "WeaponModels.h"
#include "Engine.h"
#include "ConVarRegistry.h"
#include "console/Console.h"
#include "effects/FxEmitters.h"
#include "effects/FxTuning.h"
#include "renderer/ParticleSystem.h"
#include "renderer/ShaderLibrary.h"
#include "renderer/ShaderProgram.h"
#include "renderer/SpriteSystem.h"
#include <SDL.h>
#include <windows.h>
#include <GL/gl.h>
#include <unordered_map>
#include <unordered_set>

// The director of the weapon visual effects. Each frame it
//   1. gives every replicated entity a chance to emit particles,
//   2. steps the particle simulation,
//   3. draws models, glowing geometry and finally the particles.
// What each effect looks like lives in effects/Fx*.cpp; its numbers in FxTuning.h.
// Everything is loaded from files: a missing sprite, model or shader simply means
// that part of the effect is not drawn.
namespace {

    constexpr float kMaxFrameTime = 0.05f;
    constexpr float kDefaultFrameTime = 0.016f;
    constexpr float kNewEffectMaxAge = 0.2f;   // one-shot bursts only fire for freshly created entities

    std::unordered_map<int, Fx::EmitterState> g_emitterStates;   // by entity id
    std::unordered_set<int> g_knownEntityIds;                    // ids seen last frame
    uint32_t g_lastTicks = 0;

    //--------------------------------------------------------------------------
    // Setup and diagnostics
    //--------------------------------------------------------------------------
    void LogFxStatus() {
        SpriteSystem::LogStatus();

        Console::Log(std::string("shader orb: ") + (ShaderLibrary::Get("orb") != nullptr ? "var" : "YOK"));
        Console::Log(std::string("shader pool: ") + (ShaderLibrary::Get("pool") != nullptr ? "var" : "YOK"));

        WeaponModels::LogStatus();
    }

    void InitializeOnce() {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;

        Fx::RegisterCvars();
        ShaderLibrary::Load("orb", "fullscreen");
        ShaderLibrary::Load("pool", "fullscreen");

        ConVarRegistry::RegisterCommand("fx_status",
            [](int, const char* const*) { LogFxStatus(); },
            "Lists which weapon effect files (sprites, shaders, models) were found", CVAR_NONE);
    }

    //--------------------------------------------------------------------------
    // Frame helpers
    //--------------------------------------------------------------------------
    float ElapsedSeconds() {
        const uint32_t now = SDL_GetTicks();
        const float elapsed = (g_lastTicks == 0) ? kDefaultFrameTime : static_cast<float>(now - g_lastTicks) * 0.001f;
        g_lastTicks = now;
        return Fx::Clamp(elapsed, 0.0f, kMaxFrameTime);
    }

    Fx::CameraBasis CameraFromViewMatrix() {
        const glm::dmat4 view = renderer.GetViewMatrix();

        Fx::CameraBasis camera;
        camera.right = glm::vec3(static_cast<float>(view[0][0]), static_cast<float>(view[1][0]), static_cast<float>(view[2][0]));
        camera.up = glm::vec3(static_cast<float>(view[0][1]), static_cast<float>(view[1][1]), static_cast<float>(view[2][1]));
        camera.position = glm::vec3(glm::inverse(view)[3]);
        return camera;
    }

    bool IsWeaponEffectEntity(uint16_t classId) {
        return classId == ENT_CLASS_PROJECTILE || classId >= ENT_CLASS_ROCKET;
    }

    float SeedFor(const NetEntityState& entity, int modulus) {
        if (modulus <= 0) return 0.5f;
        return static_cast<float>(entity.id % modulus) / static_cast<float>(modulus);
    }

    //--------------------------------------------------------------------------
    // Step 1: emit particles
    //--------------------------------------------------------------------------
    void EmitForEntity(const NetEntityState& entity, Fx::EmitterState& state, float deltaTime, bool justAppeared) {
        const bool freshlyCreated = justAppeared && Fx::AgeSeconds(entity) < kNewEffectMaxAge;

        switch (entity.classId) {
        case ENT_CLASS_EXPLOSION:
            if (freshlyCreated) Fx::SpawnExplosion(entity.origin, Fx::RadiusOf(entity));
            break;
        case ENT_CLASS_BFG_EXPLOSION:
            if (freshlyCreated) Fx::SpawnBfgExplosion(entity.origin, Fx::RadiusOf(entity));
            break;
        case ENT_CLASS_BEAM:
            if (freshlyCreated) Fx::SpawnRayBeamFlashes(entity);
            break;
        case ENT_CLASS_FOG_CLOUD:
            Fx::EmitFog(entity, state, deltaTime, justAppeared);
            break;
        case ENT_CLASS_LAVA_POOL:
            Fx::EmitPool(entity, state, deltaTime, justAppeared, false);
            break;
        case ENT_CLASS_ACID_POOL:
            Fx::EmitPool(entity, state, deltaTime, justAppeared, true);
            break;
        default:
            Fx::EmitProjectileTrail(entity, state, deltaTime);
            break;
        }
    }

    void EmitParticles(const std::vector<NetEntityState>& entities, float deltaTime) {
        std::unordered_set<int> currentIds;

        for (const NetEntityState& entity : entities) {
            if (!IsWeaponEffectEntity(entity.classId)) continue;

            currentIds.insert(entity.id);
            const bool justAppeared = g_knownEntityIds.find(entity.id) == g_knownEntityIds.end();
            EmitForEntity(entity, g_emitterStates[entity.id], deltaTime, justAppeared);
        }

        // Forget the state of entities that disappeared.
        for (auto it = g_emitterStates.begin(); it != g_emitterStates.end();) {
            if (currentIds.find(it->first) == currentIds.end()) it = g_emitterStates.erase(it);
            else ++it;
        }
        g_knownEntityIds.swap(currentIds);
    }

    //--------------------------------------------------------------------------
    // Step 3a: solid models (only when the GLB file exists)
    //--------------------------------------------------------------------------
    void DrawModel(const NetEntityState& entity, float timeSeconds) {
        switch (entity.classId) {
        case ENT_CLASS_ROCKET:
            glPushMatrix();
            glTranslatef(entity.origin.x, entity.origin.y, entity.origin.z);
            Fx::ApplyDirectionTransform(entity.yaw, entity.pitch);
            WeaponModels::Draw(entity.classId, timeSeconds, 0.0f);
            glPopMatrix();
            break;

        case ENT_CLASS_GRENADE:
        case ENT_CLASS_FOG_BOMB:
        case ENT_CLASS_LAVA_BOMB:
        case ENT_CLASS_ACID_BOMB:
            glPushMatrix();
            glTranslatef(entity.origin.x, entity.origin.y, entity.origin.z);
            WeaponModels::Draw(entity.classId, timeSeconds, 0.0f);
            glPopMatrix();
            break;

        case ENT_CLASS_SAWBLADE: {
            const float spin01 = static_cast<float>(entity.flags) / 100.0f;   // the spin is stored x100 in flags
            glPushMatrix();
            glTranslatef(entity.origin.x, entity.origin.y, entity.origin.z);
            WeaponModels::Draw(entity.classId, timeSeconds, spin01);
            glPopMatrix();
            break;
        }

        default:
            break;
        }
    }

    //--------------------------------------------------------------------------
    // Step 3b: glowing and translucent geometry
    //--------------------------------------------------------------------------
    void DrawGlowingGeometry(const NetEntityState& entity, float timeSeconds, const Fx::CameraBasis& camera) {
        namespace Orb = FxTuning::Orb;

        switch (entity.classId) {
        case ENT_CLASS_STAPLE_BALL:
            Fx::DrawEnergyOrb(entity.origin, Orb::kStapleBall.size, Orb::kStapleBall.color,
                SeedFor(entity, Orb::kStapleBall.seedModulus), Orb::kStapleBall.spikes, timeSeconds, camera);
            break;

        case ENT_CLASS_PROJECTILE:
            Fx::DrawEnergyOrb(entity.origin, Orb::kBotBall.size, Orb::kBotBall.color,
                SeedFor(entity, Orb::kBotBall.seedModulus), Orb::kBotBall.spikes, timeSeconds, camera);
            break;

        case ENT_CLASS_LAST_BALL:
            Fx::DrawEnergyOrb(entity.origin, Orb::kLastBall.size, Orb::kLastBall.color,
                SeedFor(entity, Orb::kLastBall.seedModulus), Orb::kLastBall.spikes, timeSeconds, camera);
            break;

        case ENT_CLASS_BEAM:
            Fx::DrawRayBeam(entity, camera);
            break;

        case ENT_CLASS_LAVA_POOL:
        case ENT_CLASS_ACID_POOL: {
            auto state = g_emitterStates.find(entity.id);
            if (state != g_emitterStates.end()) {
                Fx::DrawLiquidPool(entity, state->second, entity.classId == ENT_CLASS_ACID_POOL, timeSeconds);
            }
            break;
        }

        default:
            break;
        }
    }

    bool AnyEffectToDraw(const std::vector<NetEntityState>& entities) {
        if (!ParticleSystem::Empty()) return true;

        for (const NetEntityState& entity : entities) {
            if (IsWeaponEffectEntity(entity.classId)) return true;
        }
        return false;
    }

}  // namespace

void WeaponEffects_Render(const std::vector<NetEntityState>& entities) {
    InitializeOnce();

    const float deltaTime = ElapsedSeconds();
    const float timeSeconds = static_cast<float>(SDL_GetTicks()) * 0.001f;

    EmitParticles(entities, deltaTime);
    ParticleSystem::Update(deltaTime);

    if (!AnyEffectToDraw(entities)) return;

    const Fx::CameraBasis camera = CameraFromViewMatrix();

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    for (const NetEntityState& entity : entities) {
        DrawModel(entity, timeSeconds);
    }

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    for (const NetEntityState& entity : entities) {
        DrawGlowingGeometry(entity, timeSeconds, camera);
    }

    glPopAttrib();

    // Particles last: they are sorted and blended on top of everything above.
    ParticleSystem::Render(camera.position, camera.right, camera.up);
}