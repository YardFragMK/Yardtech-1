#include "BotStilar.h"
#include "BotManager.h"
#include "Player.h"
#include "../../engine/Camera.h"
#include "BSPMap.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr int HEALTH_BY_DIFFICULTY[4] = { 70, 90, 100, 150 };
    constexpr int DAMAGE_BY_DIFFICULTY[4] = { 15, 25, 40, 50 };
}

void BotStilar::SpawnAt(const glm::vec3& spawnPosition, int difficulty) {
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 3) difficulty = 3;

    position = spawnPosition;
    m_groundY = spawnPosition.y;
    health = HEALTH_BY_DIFFICULTY[difficulty];
    damage = DAMAGE_BY_DIFFICULTY[difficulty];
    m_state = State::Cooldown;
    m_stateTimer = COOLDOWN_TIME;
    m_shotsFiredThisBurst = 0;

    Spawn();
}

void BotStilar::Spawn() {}

void BotStilar::Live(float deltaTime) {
    if (!IsAlive()) return;

    Run(deltaTime);

    if (m_state == State::Jumping) {
        UpdateJump(deltaTime);
        return;
    }

    m_stateTimer -= deltaTime;

    if (m_state == State::Cooldown) {
        if (m_stateTimer <= 0.0f) {
            m_state = State::Firing;
            m_stateTimer = 0.0f;
            m_shotsFiredThisBurst = 0;
        }
    }
    else if (m_state == State::Firing) {
        if (m_stateTimer <= 0.0f) {
            Shoot();
            m_shotsFiredThisBurst++;
            m_stateTimer = FIRE_INTERVAL;

            if (m_shotsFiredThisBurst >= BURST_COUNT) {
                StartJumpTowardPlayer();
            }
        }
    }
}

void BotStilar::StartJumpTowardPlayer() {
    glm::vec3 toPlayer = g_Camera.GetEyePosition() - position;
    toPlayer.y = 0.0f;

    glm::vec3 horizontalDir(0.0f, 0.0f, 1.0f);
    if (glm::length(toPlayer) > 0.0001f) {
        horizontalDir = glm::normalize(toPlayer);
    }

    m_jumpVelocity = horizontalDir * JUMP_HORIZONTAL_SPEED + glm::vec3(0.0f, JUMP_VERTICAL_SPEED, 0.0f);
    m_contactDamageDealtThisJump = false;
    m_state = State::Jumping;
}

void BotStilar::UpdateJump(float deltaTime) {
    m_jumpVelocity.y -= JUMP_GRAVITY * deltaTime;

    glm::vec3 oldPos = position;
    glm::vec3 desiredNewPos = oldPos + m_jumpVelocity * deltaTime;

    // Botun dunya geometrisine gore hareketi collision'lu cozuluyor -- ayni
    // SlideMove mantigi oyuncu icin de kullaniliyor. hullIndex=1 (ayakta
    // duran standart insan hull'u), botun boyutuna tam tam uyan ozel bir
    // hull tanimlanmadigi icin en yakin, makul yaklasim bu.
    glm::vec3 resolvedPos = g_Map.SlideMove(oldPos, desiredNewPos, 1);
    position = resolvedPos;

    // Dikeyde bir carpisma oldu mu (zemine indi ya da tavana carpti) diye
    // kontrol ediyoruz -- bu, ziplama hizini sifirlamamiz gereken an.
    bool verticalBlocked = std::abs(resolvedPos.y - desiredNewPos.y) > 0.01f;

    if (!m_contactDamageDealtThisJump) {
        float distToPlayer = glm::length(position - g_Camera.GetEyePosition());
        if (distToPlayer < CONTACT_RADIUS) {
            g_Player.takeDamage(damage);
            m_contactDamageDealtThisJump = true;
        }
    }

    // Inis: ya baslangic zeminine ya da collision nedeniyle asagi yonlu
    // hareketin durdugu bir noktaya dustuysek, ziplama biter.
    bool landedOnGround = (position.y <= m_groundY) || (verticalBlocked && m_jumpVelocity.y < 0.0f);

    if (landedOnGround) {
        position.y = std::max<>(position.y, m_groundY);
        m_jumpVelocity = glm::vec3(0.0f);
        m_state = State::Cooldown;
        m_stateTimer = COOLDOWN_TIME;
    }
}

void BotStilar::Run(float deltaTime) {
    (void)deltaTime;
}

void BotStilar::Shoot() {
    glm::vec3 target = g_Camera.GetEyePosition();
    glm::vec3 toTarget = target - position;

    if (glm::length(toTarget) < 0.0001f) return;
    glm::vec3 direction = glm::normalize(toTarget);

    float speed = g_Camera.moveSpeed;

    BotManager::SpawnProjectile(position, direction, speed, damage);
}

void BotStilar::Render() const {
    if (!IsAlive()) return;
    RenderModelOrBox();
}