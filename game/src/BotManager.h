#pragma once
#include <vector>
#include <memory>
#include "e_BotBase.h"
#include "BotProjectile.h"

class BotManager {
public:
    // Haritanin entity listesini tarar, "monster_stilar" classname'ine sahip
    // her entity icin bir BotStilar spawn eder. Entity'nin "origin" ve
    // "difficulty" key'leri okunur; difficulty yoksa varsayilan (1) kullanilir.
    static void Init();

    static void Update(float deltaTime);
    static void Render();

    static void SpawnProjectile(const glm::vec3& origin, const glm::vec3& direction, float speed, int damage);

private:
    static std::vector<std::unique_ptr<e_BotBase>> s_bots;
    static std::vector<BotProjectile> s_projectiles;

    static void UpdateProjectiles(float deltaTime);
    static void RenderProjectiles();
};