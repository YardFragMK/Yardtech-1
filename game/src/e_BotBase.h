#pragma once
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>

// Tum botlarin ortak arayuzu. Alt siniflar (orn. BotStilar) kendi Spawn/Live/
// Run/Shoot davranisini tanimlar. position/health/damage/halfExtents gibi
// alanlar botlarin cogunda ortak oldugu icin burada tutulur; BotManager bu
// taban sinif uzerinden polymorphic olarak calisir.
class e_BotBase {
public:
    virtual ~e_BotBase() = default;

    virtual void Spawn();
    virtual void Live(float deltaTime);  // Botun ana dusunme fonksiyonu, her frame cagrilir
    virtual void Run(float deltaTime);   // Botun harita icerisindeki hareket fonksiyonu
    virtual void Shoot();                // Botun dunya ile etkilesim (atis) fonksiyonu
    virtual void Render() const;         // Botun gorsel temsili

    bool IsAlive() const { return health > 0; }

protected:
    glm::vec3 position{ 0.0f };
    glm::vec3 halfExtents{ 16.0f, 32.0f, 16.0f }; // dikdortgen prizmanin yari-boyutlari
    int health = 100;
    int damage = 15;
};