#include "BotStilar.h"
#include "BotManager.h"
#include "../../engine/Camera.h"
#include <windows.h>
#include <GL/gl.h>

namespace {
    // Zorluk seviyesine gore artan can/hasar degerleri.
    constexpr int HEALTH_BY_DIFFICULTY[4] = { 70, 90, 100, 150 };
    constexpr int DAMAGE_BY_DIFFICULTY[4] = { 15, 25, 40, 50 };
}

void BotStilar::SpawnAt(const glm::vec3& spawnPosition, int difficulty) {
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 3) difficulty = 3;

    position = spawnPosition;
    health = HEALTH_BY_DIFFICULTY[difficulty];
    damage = DAMAGE_BY_DIFFICULTY[difficulty];
    m_state = State::Cooldown;
    m_stateTimer = COOLDOWN_TIME;
    m_shotsFiredThisBurst = 0;

    Spawn();
}

void BotStilar::Spawn() {
    // Su an ekstra bir kurulum gerekmiyor -- ileride spawn animasyonu/sesi
    // buraya eklenebilir.
}

void BotStilar::Live(float deltaTime) {
    if (!IsAlive()) return;

    Run(deltaTime);

    m_stateTimer -= deltaTime;

    if (m_state == State::Cooldown) {
        if (m_stateTimer <= 0.0f) {
            m_state = State::Firing;
            m_stateTimer = 0.0f; // ilk atis hemen yapilsin
            m_shotsFiredThisBurst = 0;
        }
    }
    else if (m_state == State::Firing) {
        if (m_stateTimer <= 0.0f) {
            Shoot();
            m_shotsFiredThisBurst++;
            m_stateTimer = FIRE_INTERVAL;

            if (m_shotsFiredThisBurst >= BURST_COUNT) {
                m_state = State::Cooldown;
                m_stateTimer = COOLDOWN_TIME;
            }
        }
    }
}

void BotStilar::Run(float deltaTime) {
    // Simdilik Stilar sabit duran bir "turret" -- ileride devriye/takip
    // hareketi buraya eklenebilir.
    (void)deltaTime;
}

void BotStilar::Shoot() {
    glm::vec3 target = g_Camera.GetEyePosition();
    glm::vec3 toTarget = target - position;

    if (glm::length(toTarget) < 0.0001f) return;
    glm::vec3 direction = glm::normalize(toTarget);

    // Hiz, atisin yapildigi anki oyuncu hareket hizina kilitlenir.
    float speed = g_Camera.moveSpeed;

    BotManager::SpawnProjectile(position, direction, speed, damage);
}

void BotStilar::Render() const {
    if (!IsAlive()) return;

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.35f, 0.05f, 0.05f, 1.0f); // koyu kirmizimsi, tehditkar bir renk

    float hx = halfExtents.x, hy = halfExtents.y, hz = halfExtents.z;

    glBegin(GL_QUADS);
    glVertex3f(hx, -hy, -hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, hy, -hz);
    glVertex3f(-hx, -hy, hz); glVertex3f(-hx, -hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, hy, hz);
    glVertex3f(-hx, hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(hx, hy, hz); glVertex3f(-hx, hy, hz);
    glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, -hy, -hz); glVertex3f(-hx, -hy, -hz);
    glVertex3f(-hx, -hy, hz); glVertex3f(-hx, hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, -hy, hz);
    glVertex3f(hx, -hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, -hy, -hz);
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}