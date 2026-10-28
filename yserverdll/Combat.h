#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "netentity.h"

// Oyuncu silahlarinin sunucu tarafi savas sistemi: anlik vurus (hitscan), mermiler,
// patlamalar, sis bulutu ve lav havuzu. Hasar botlara ve (cok oyunculuda) diger
// oyunculara uygulanir.
namespace Combat {
    enum BombKind { BOMB_GRENADE = 0, BOMB_FOG = 1, BOMB_LAVA = 2, BOMB_ACID = 3, BOMB_KIND_COUNT = 4 };
    void Reset();
    void Update(float deltaTime);

    // Mermileri ve etkileri replikasyon icin entity durumlarina cevirir.
    int CollectEntityStates(NetEntityState* outStates, int maxCount);

    // Namlu cikisi: goz konumundan yone distance kadar ilerler, duvara denk gelirse oraya kadar.
    glm::vec3 MuzzlePoint(const glm::vec3& eye, const glm::vec3& direction, float distance);

    // Anlik vurus: pellets adet isin, spreadDegrees koni acisi icinde rastgele dagilir.
    // En yakin hedefe hasar verilir; kafaya (hedef kutusunun ust ceyregi) headshotMultiplier uygulanir.
    void FireBullets(int owner, const glm::vec3& eye, const glm::vec3& forward,
        const glm::vec3& right, const glm::vec3& up,
        int pellets, float spreadDegrees, int damagePerPellet, float headshotMultiplier);

    // Her seyi delen isin (duvarlar haric): yoldaki tum hedeflere hasar verir.
    void FireRay(int owner, const glm::vec3& eye, const glm::vec3& forward, int damage);

    void SpawnRocket(int owner, const glm::vec3& position, const glm::vec3& direction);
    void SpawnBomb(int owner, const glm::vec3& position, const glm::vec3& direction, int bombKind);
    void SpawnSawblade(int owner, const glm::vec3& position, const glm::vec3& direction, float spin, int damage);
    void SpawnStapleBall(int owner, const glm::vec3& position, const glm::vec3& direction);
    void SpawnLastBall(int owner, const glm::vec3& position, const glm::vec3& direction);

    int PickRandomBombKind();

    // Nokta bir sis bulutunun icinde mi? (botlar sisteki oyuncuyu goremez)
    bool IsPositionObscured(const glm::vec3& position);
}