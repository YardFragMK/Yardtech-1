#pragma once
#include <glm/glm.hpp>

// Ileri bildirim: gercek tanim BSPMap.h icinde. Bu dosyanin BSPMap'e
// header seviyesinde bagimli olmasina gerek yok, sadece referans/pointer
// gecirilecek.
class BSPMap;

// Bir oyuncunun fizik simulasyonu icin gereken tum durumu tasir. Ne client'a
// ne server'a ozgu bir kavram icermez -- sadece sayilar ve bayraklar. Hem
// server'in yetkili simulasyonunda hem client'in tahmininde (prediction)
// ayni struct kullanilir.
struct PlayerPhysicsState {
    glm::vec3 position{ 0.0f };
    float verticalVelocity = 0.0f;
    bool onGround = false;
    bool isCrouching = false;

    bool airborneFromJump = false;
    bool airborneFromHardSlam = false;
};

// O anki tick'te oyuncunun ne yapmak istedigini tasir. Client bunu klavyeden
// okuyup gonderir; server aynen bu komutu kullanarak simulasyonu kosturur --
// boylece iki taraf da ayni girdiyle ayni sonuca ulasir (deterministik).
struct PlayerInputCommand {
    bool jumpPressed = false;   // bu tick'te YENI basildi mi (latch, cagiran taraf hesaplar)
    bool ctrlPressed = false;   // bu tick'te YENI basildi mi
    bool ctrlHeld = false;      // su an basili mi
    bool noclip = false;
    bool consoleOpen = false;   // konsol/menu acikken fizik dondurulur
    float yaw = 0.0f;           // bakis yonu (yatay hareket vektorunu hesaplamak icin)
    float moveForwardAxis = 0.0f; // -1..1, W/S
    float moveRightAxis = 0.0f;   // -1..1, D/A
    float moveSpeed = 320.0f;
    bool canHardSlam = true;    // eski "g_Player.RGDitem" karsiligi, cagiran taraf karar verir
};

// Fizigin bu tick'te urettigi olaylar. Cagiran taraf (client) bunlari gorsel
// efektler (landing shake gibi) tetiklemek icin kullanir; bu struct'in
// kendisi hicbir gorsel kavram icermez, sadece "ne oldu" bilgisini tasir.
struct PlayerPhysicsEvents {
    bool jumpLanded = false;
    bool hardSlamLanded = false;
};

// GoldSrc hull yari-yukseklikleri. Baska dosyalarin da (orn. client-side
// render/eye-height hesaplari) ihtiyac duyabilecegi sabitler oldugu icin
// burada, public olarak tanimli.
namespace PlayerPhysicsConstants {
    constexpr float HALF_STAND = 36.0f;
    constexpr float HALF_CROUCH = 18.0f;
    constexpr float HULL_DIFF = HALF_STAND - HALF_CROUCH;
    constexpr float FALL_GRAVITY_MULTIPLIER = 1.6f;
    constexpr float HARD_SLAM_VELOCITY_MULTIPLIER = 4.0f;
}

// Bir tick'lik fizik simulasyonunu kosturur. state icindeki degerleri yerinde
// gunceller, o tick'te olusan olaylari dondurur. Hicbir global degiskene
// (kamera, konsol, cvar, SDL) dokunmaz -- tum girdisini parametrelerden alir.
PlayerPhysicsEvents SimulatePlayerPhysics(
    PlayerPhysicsState& state,
    const PlayerInputCommand& input,
    BSPMap& map,
    float deltaTime,
    float gravity,
    float jumpforce
);