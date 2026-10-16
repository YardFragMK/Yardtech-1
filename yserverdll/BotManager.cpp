#include "pch.h"
#include "BotManager.h"
#include "BotStilar.h"
#include "ServerContext.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

std::vector<std::unique_ptr<e_BotBase>> BotManager::s_bots;
std::vector<BotProjectile> BotManager::s_projectiles;
int32_t BotManager::s_nextEntityId = 1;

namespace {
    constexpr const char* BOT_STILAR_CLASSNAME = "bot_stilar";

    // BSP "origin" metnini ("x y z") motor uzayina (Y-up) cevirir; BSP'nin
    // Z-up eksenlerinden (x, z, -y) donusumu uygulanir.
    glm::vec3 ParseBspOrigin(const char* text) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        std::istringstream stream(text);
        stream >> x >> y >> z;
        return glm::vec3(x, z, -y);
    }
}

void BotManager::Init() {
    Shutdown();

    IWorldQuery* world = Ctx().world;
    if (world == nullptr) return;

    int spawned = 0;
    const int entityCount = world->GetEntityCount();

    for (int i = 0; i < entityCount; i++) {
        const char* classname = world->GetEntityKeyValue(i, "classname");
        if (classname == nullptr || std::strcmp(classname, BOT_STILAR_CLASSNAME) != 0) continue;

        const char* origin = world->GetEntityKeyValue(i, "origin");
        if (origin == nullptr) continue;

        const char* difficultyText = world->GetEntityKeyValue(i, "difficulty");
        int difficulty = (difficultyText != nullptr) ? std::atoi(difficultyText) : 1;

        auto stilar = std::make_unique<BotStilar>();
        stilar->SpawnAt(ParseBspOrigin(origin), difficulty, s_nextEntityId++);
        s_bots.push_back(std::move(stilar));
        spawned++;
    }

    Ctx().Log(("server.dll: " + std::to_string(spawned) + " bot olusturuldu").c_str());
}

void BotManager::Shutdown() {
    s_bots.clear();
    s_projectiles.clear();
    s_nextEntityId = 1;
}

void BotManager::Update(float deltaTime) {
    for (auto& bot : s_bots) {
        if (bot->IsAlive()) {
            bot->Live(deltaTime);
        }
    }

    UpdateProjectiles(deltaTime);
}

void BotManager::UpdateProjectiles(float deltaTime) {
    constexpr float HIT_RADIUS = 24.0f;
    IWorldQuery* world = Ctx().world;

    for (auto& proj : s_projectiles) {
        if (!proj.alive) continue;

        glm::vec3 oldPos = proj.position;
        glm::vec3 newPos = oldPos + proj.direction * proj.speed * deltaTime;

        // Duvara ya da zemine carpan mermi yok olur (nokta hull).
        if (world != nullptr) {
            TraceResult trace = world->TraceLine(oldPos, newPos, 0);
            if (trace.fraction < 1.0f) {
                proj.alive = false;
                continue;
            }
        }

        proj.position = newPos;

        for (PlayerSlot& slot : Ctx().slots) {
            if (!slot.connected) continue;

            if (glm::length(proj.position - slot.eyePosition) < HIT_RADIUS) {
                slot.player.TakeDamage(proj.damage);
                proj.alive = false;
                break;
            }
        }
    }

    s_projectiles.erase(
        std::remove_if(s_projectiles.begin(), s_projectiles.end(),
            [](const BotProjectile& p) { return !p.alive; }),
        s_projectiles.end()
    );
}

void BotManager::SpawnProjectile(const glm::vec3& origin, const glm::vec3& direction, float speed, int damage) {
    BotProjectile proj;
    proj.id = s_nextEntityId++;
    proj.position = origin;
    proj.direction = direction;
    proj.speed = speed;
    proj.damage = damage;
    proj.alive = true;
    s_projectiles.push_back(proj);
}

int BotManager::CollectEntityStates(NetEntityState* outStates, int maxCount) {
    int count = 0;

    for (const auto& bot : s_bots) {
        if (count >= maxCount) break;
        if (!bot->IsAlive()) continue;

        NetEntityState& state = outStates[count++];
        state = NetEntityState();
        state.id = bot->GetEntityId();
        state.classId = bot->GetClassId();
        state.health = static_cast<int16_t>(bot->GetHealth());
        state.origin = bot->GetPosition();
    }

    for (const auto& proj : s_projectiles) {
        if (count >= maxCount) break;
        if (!proj.alive) continue;

        NetEntityState& state = outStates[count++];
        state = NetEntityState();
        state.id = proj.id;
        state.classId = ENT_CLASS_PROJECTILE;
        state.origin = proj.position;
        state.yaw = glm::degrees(std::atan2(proj.direction.z, proj.direction.x));
        state.pitch = glm::degrees(std::asin(glm::clamp(proj.direction.y, -1.0f, 1.0f)));
    }

    return count;
}