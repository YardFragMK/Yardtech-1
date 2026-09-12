#pragma once
#include <vector>
#include <memory>
#include "e_BotBase.h"
#include "BotProjectile.h"

// Sahnedeki tum botlari ve onlarin projektillerini yonetir. Su asamada
// botlar haritanin entity sistemine bagli degil -- sabit test konumlarinda
// olusturuluyor. Ileride harita entity'lerinden (orn. bir "bot_spawn"
// siniflandirmasi) otomatik spawn eklenebilir.
class BotManager {
public:
    static void Init();
    static void Update(float deltaTime);
    static void Render();

    // Bot alt siniflari (BotStilar::Shoot gibi) yeni bir projektil olusturmak
    // icin bunu cagirir.
    static void SpawnProjectile(const glm::vec3& origin, const glm::vec3& direction, float speed, int damage);

private:
    static std::vector<std::unique_ptr<e_BotBase>> s_bots;
    static std::vector<BotProjectile> s_projectiles;

    static void UpdateProjectiles(float deltaTime);
    static void RenderProjectiles();
};