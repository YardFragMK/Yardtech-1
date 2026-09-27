#pragma once
#include <glm/glm.hpp>
#include <windows.h>
#include <GL/gl.h>
#include "GLBModel.h"

// Tum botlarin ortak arayuzu. Alt siniflar (orn. BotStilar) kendi Spawn/Live/
// Run/Shoot davranisini tanimlar. position/health/damage/halfExtents gibi
// alanlar botlarin cogunda ortak oldugu icin burada tutulur; BotManager bu
// taban sinif uzerinden polymorphic olarak calisir.
class e_BotBase {
public:
    virtual ~e_BotBase() = default;

    virtual void Spawn()=0;
    virtual void Live(float deltaTime)=0;
    virtual void Run(float deltaTime)=0;
    virtual void Shoot()=0;
    virtual void Render() const=0;

    bool IsAlive() const { return health > 0; }

    // Modeli disaridan (BotManager) yuklemek icin. Model bulunamazsa/yuklenemezse
    // Render() otomatik olarak eski kutu-cizim davranisina doner.
    bool LoadModel(const std::string& glbPath) { return m_model.Load(glbPath); }

protected:
    glm::vec3 position{ 0.0f };
    glm::vec3 halfExtents{ 16.0f, 32.0f, 16.0f };
    int health = 100;
    int damage = 15;

    GLBModel m_model;

    // Turetilmis siniflar bu ortak cizim yardimcisini kullanabilir: model
    // yuklenmisse onu, yuklenmemisse eski placeholder kutuyu cizer.
    void RenderModelOrBox() const;
};