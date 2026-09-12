#pragma once
#include "e_BotBase.h"

// Doom'daki Imp'e benzer bir enerji-topu dusmani: bekler, oyuncuya kilitlenip
// seri (art arda) enerji toplari firlatir, sonra tekrar bekler. Her top,
// atildigi anda oyuncunun bulundugu yone kilitlenir ve o sabit dogrultuda
// duz gider -- oyuncuyu takip etmez, bu yuzden oyuncu hareket ederek kacabilir.
class BotStilar : public e_BotBase {
public:
    // difficulty: 0=kolay .. 3=zor, Health/Damage tablosundan deger secer.
    void SpawnAt(const glm::vec3& spawnPosition, int difficulty);

    void Spawn() override;
    void Live(float deltaTime) override;
    void Run(float deltaTime) override;
    void Shoot() override;
    void Render() const override;

private:
    enum class State { Cooldown, Firing };
    State m_state = State::Cooldown;
    float m_stateTimer = 0.0f;
    int m_shotsFiredThisBurst = 0;

    static constexpr int BURST_COUNT = 6;
    static constexpr float FIRE_INTERVAL = 0.15f;
    static constexpr float COOLDOWN_TIME = 2.5f;
};