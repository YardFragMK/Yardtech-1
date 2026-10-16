#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "e_BotBase.h"
#include "BotProjectile.h"
#include "netentity.h"

class BotManager {
public:
    // Dunyanin entity listesini tarar; "bot_stilar" siniflarinin her biri icin
    // bir bot olusturur. "origin" ve "difficulty" anahtarlari okunur.
    static void Init();
    static void Shutdown();

    static void Update(float deltaTime);
    static void SpawnProjectile(const glm::vec3& origin, const glm::vec3& direction, float speed, int damage);

    // Canli bot ve mermileri replikasyon icin entity durumlarina cevirir.
    static int CollectEntityStates(NetEntityState* outStates, int maxCount);

private:
    static std::vector<std::unique_ptr<e_BotBase>> s_bots;
    static std::vector<BotProjectile> s_projectiles;
    static int32_t s_nextEntityId;

    static void UpdateProjectiles(float deltaTime);
};