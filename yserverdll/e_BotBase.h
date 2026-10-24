#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "netentity.h"

// Tum botlarin ortak arayuzu. Yalnizca oyun mantigini icerir; gorsel temsil
// client tarafinda, server'in bildirdigi entity durumundan cizilir.
class e_BotBase {
public:
    virtual ~e_BotBase() = default;

    virtual void Spawn();
    virtual void Live(float deltaTime);   // botun ana dusunme fonksiyonu
    virtual void Run(float deltaTime);    // harita icindeki hareket
    virtual void Shoot();                 // dunya ile etkilesim (atis)
    virtual uint16_t GetClassId() const;

    // Oyuncu silahlarindan gelen hasar.
    virtual void TakeDamage(int amount);

    bool IsAlive() const { return health > 0; }
    const glm::vec3& GetPosition() const { return position; }
    // Vurus kutusu: position merkezli, +-halfExtents. Kafa, kutunun ust ceyregidir.
    const glm::vec3& GetHalfExtents() const { return halfExtents; }
    int GetHealth() const { return health; }
    int32_t GetEntityId() const { return entityId; }
    void SetEntityId(int32_t id) { entityId = id; }

protected:
    glm::vec3 position{ 0.0f };
    glm::vec3 halfExtents{ 16.0f, 32.0f, 16.0f };
    int health = 100;
    int damage = 15;
    int32_t entityId = 0;
};