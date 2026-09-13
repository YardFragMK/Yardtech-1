#pragma once
#include "e_BotBase.h"

class BotStilar : public e_BotBase {
public:
    void SpawnAt(const glm::vec3& spawnPosition, int difficulty);

    void Spawn() override;
    void Live(float deltaTime) override;
    void Run(float deltaTime) override;
    void Shoot() override;
    void Render() const override;

private:
    enum class State { Cooldown, Firing, Jumping };
    State m_state = State::Cooldown;
    float m_stateTimer = 0.0f;
    int m_shotsFiredThisBurst = 0;

    glm::vec3 m_jumpVelocity{ 0.0f };
    float m_groundY = 0.0f;
    bool m_contactDamageDealtThisJump = false;

    static constexpr int BURST_COUNT = 6;
    static constexpr float FIRE_INTERVAL = 0.15f;
    static constexpr float COOLDOWN_TIME = 2.5f;

    static constexpr float JUMP_HORIZONTAL_SPEED = 220.0f;
    static constexpr float JUMP_VERTICAL_SPEED = 260.0f;
    static constexpr float JUMP_GRAVITY = 700.0f;
    static constexpr float CONTACT_RADIUS = 40.0f;

    void StartJumpTowardPlayer();
    void UpdateJump(float deltaTime);
};