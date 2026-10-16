#include "pch.h"
#include "BotStilar.h"
#include "BotManager.h"
#include "ServerContext.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr int HEALTH_BY_DIFFICULTY[4] = { 70, 90, 100, 150 };
    constexpr int DAMAGE_BY_DIFFICULTY[4] = { 15, 25, 40, 50 };
}

void BotStilar::SpawnAt(const glm::vec3& spawnPosition, int difficulty, int32_t id) {
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 3) difficulty = 3;

    entityId = id;
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

uint16_t BotStilar::GetClassId() const { return ENT_CLASS_BOT_STILAR; }

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
    glm::vec3 horizontalDir(0.0f, 0.0f, 1.0f);

    if (PlayerSlot* target = Ctx().FindNearestPlayer(position)) {
        glm::vec3 toPlayer = target->eyePosition - position;
        toPlayer.y = 0.0f;
        if (glm::length(toPlayer) > 0.0001f) {
            horizontalDir = glm::normalize(toPlayer);
        }
    }

    m_jumpVelocity = horizontalDir * JUMP_HORIZONTAL_SPEED + glm::vec3(0.0f, JUMP_VERTICAL_SPEED, 0.0f);
    m_contactDamageDealtThisJump = false;
    m_state = State::Jumping;
}

void BotStilar::UpdateJump(float deltaTime) {
    m_jumpVelocity.y -= JUMP_GRAVITY * deltaTime;

    glm::vec3 oldPos = position;
    glm::vec3 desiredNewPos = oldPos + m_jumpVelocity * deltaTime;

    // Hareket, oyuncuyla ayni SlideMove mantigiyla collision'li cozulur.
    // Bota ozel bir hull tanimlanmadigi icin ayakta duran standart hull (1)
    // en yakin makul yaklasimdir.
    glm::vec3 resolvedPos = desiredNewPos;
    if (IWorldQuery* world = Ctx().world) {
        resolvedPos = world->SlideMove(oldPos, desiredNewPos, 1);
    }
    position = resolvedPos;

    // Dikeyde bir engel (zemin/tavan) olustu mu; ziplama hizini sifirlama ani.
    bool verticalBlocked = std::abs(resolvedPos.y - desiredNewPos.y) > 0.01f;

    if (!m_contactDamageDealtThisJump) {
        PlayerSlot* target = Ctx().FindNearestPlayer(position);
        if (target != nullptr && glm::length(position - target->eyePosition) < CONTACT_RADIUS) {
            target->player.TakeDamage(damage);
            m_contactDamageDealtThisJump = true;
        }
    }

    // Inis: baslangic zeminine donuldu ya da asagi yonlu hareket engellendi.
    bool landedOnGround = (position.y <= m_groundY) || (verticalBlocked && m_jumpVelocity.y < 0.0f);

    if (landedOnGround) {
        position.y = std::max(position.y, m_groundY);
        m_jumpVelocity = glm::vec3(0.0f);
        m_state = State::Cooldown;
        m_stateTimer = COOLDOWN_TIME;
    }
}

void BotStilar::Run(float deltaTime) {
    (void)deltaTime;
}

void BotStilar::Shoot() {
    PlayerSlot* target = Ctx().FindNearestPlayer(position);
    if (target == nullptr) return;

    glm::vec3 toTarget = target->eyePosition - position;
    if (glm::length(toTarget) < 0.0001f) return;
    glm::vec3 direction = glm::normalize(toTarget);

    // Top hizi, atis anindaki hedef oyuncunun hareket hizina esitlenir.
    BotManager::SpawnProjectile(position, direction, target->moveSpeed, damage);
}