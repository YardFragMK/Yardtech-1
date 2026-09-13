#include "BotManager.h"
#include "BotStilar.h"
#include "Player.h"
#include "../../engine/Camera.h"
#include "BSPMap.h"
#include "EntityParser.h"
#include <algorithm>
#include <cstdlib>
#include <windows.h>
#include <GL/gl.h>

std::vector<std::unique_ptr<e_BotBase>> BotManager::s_bots;
std::vector<BotProjectile> BotManager::s_projectiles;

namespace {
    constexpr const char* MONSTER_STILAR_CLASSNAME = "monster_stilar";
    constexpr const char* STILAR_MODEL_PATH = "nvs1/models/stilar/stilar.glb";
}

void BotManager::Init() {
    s_bots.clear();
    s_projectiles.clear();

    int spawnedCount = 0;

    for (const Entity& ent : g_Map.GetEntities()) {
        if (!ent.Is(MONSTER_STILAR_CLASSNAME)) continue;

        const std::string* originStr = ent.Get(EntityKeys::Origin);
        if (!originStr) continue;

        glm::vec3 spawnPos = BSPMap::ParseOriginToEngineSpace(*originStr);

        int difficulty = ent.GetInt("difficulty", 1);

        auto stilar = std::make_unique<BotStilar>();
        stilar->SpawnAt(spawnPos, difficulty);

        if (!stilar->LoadModel(STILAR_MODEL_PATH)) {
            // Model yuklenemezse RenderModelOrBox otomatik olarak eski
            // placeholder kutuya doner -- burada ayrica bir islem gerekmiyor,
            // sadece bilgilendirme amacli.
        }

        s_bots.push_back(std::move(stilar));
        spawnedCount++;
    }

    // Haritada hic monster_stilar yoksa (orn. eski test haritalarinda),
    // eskisi gibi tek bir test botu spawn ediliyor -- boylece bot sistemi
    // hala test edilebilir kaliyor.
    if (spawnedCount == 0) {
        auto stilar = std::make_unique<BotStilar>();
        stilar->SpawnAt(glm::vec3(0.0f, 60.0f, 300.0f), 1);
        stilar->LoadModel(STILAR_MODEL_PATH);
        s_bots.push_back(std::move(stilar));
    }
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

    for (auto& proj : s_projectiles) {
        if (!proj.alive) continue;

        glm::vec3 oldPos = proj.position;
        glm::vec3 newPos = oldPos + proj.direction * proj.speed * deltaTime;

        TraceResult trace = g_Map.TraceLine(oldPos, newPos, 0);
        if (trace.fraction < 1.0f) {
            proj.alive = false;
            continue;
        }

        proj.position = newPos;

        float distToPlayer = glm::length(proj.position - g_Camera.GetEyePosition());
        if (distToPlayer < HIT_RADIUS) {
            g_Player.takeDamage(proj.damage);
            proj.alive = false;
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
    proj.position = origin;
    proj.direction = direction;
    proj.speed = speed;
    proj.damage = damage;
    proj.alive = true;
    s_projectiles.push_back(proj);
}

void BotManager::Render() {
    for (const auto& bot : s_bots) {
        bot->Render();
    }
    RenderProjectiles();
}

void BotManager::RenderProjectiles() {
    for (const auto& proj : s_projectiles) {
        if (!proj.alive) continue;

        glPushMatrix();
        glTranslatef(proj.position.x, proj.position.y, proj.position.z);

        glDisable(GL_TEXTURE_2D);
        glColor4f(1.0f, 0.4f, 0.1f, 1.0f);

        const float s = 6.0f;
        glBegin(GL_QUADS);
            glVertex3f( s, -s, -s); glVertex3f( s, -s,  s); glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);
            glVertex3f(-s, -s,  s); glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s);
            glVertex3f(-s,  s, -s); glVertex3f( s,  s, -s); glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);
            glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s); glVertex3f( s, -s, -s); glVertex3f(-s, -s, -s);
            glVertex3f(-s, -s,  s); glVertex3f(-s,  s,  s); glVertex3f( s,  s,  s); glVertex3f( s, -s,  s);
            glVertex3f( s, -s, -s); glVertex3f( s,  s, -s); glVertex3f(-s,  s, -s); glVertex3f(-s, -s, -s);
        glEnd();

        glEnable(GL_TEXTURE_2D);
        glPopMatrix();
    }
}