#pragma once
#include <cstdint>
#include <cstddef>
#include "PlayerMovement.h"

// Istemci ve sunucu arasinda degisecek mesajlarin ortak tanimlarini tutar.
// Her iki taraf da bu sabitleri kullandigi surece ayni protokolde konusur;
// biri degisirse diger tarafin da guncellenmesi gerekir.
namespace NetProtocol {
    constexpr uint16_t SERVER_PORT = 27015;
    constexpr size_t MAX_CLIENTS = 16;
    constexpr size_t CHANNEL_COUNT = 2; // kanal 0: guvenilir/sirali, kanal 1: hizli/guvenilmez
    constexpr float TICK_RATE_HZ = 60.0f;
    constexpr float FIXED_DELTA_TIME = 1.0f / TICK_RATE_HZ;

    enum class MessageType : uint8_t {
        ClientHello = 1,
        ServerWelcome = 2,
        PlayerInput = 3,  // client -> server, kanal 1 (guvenilmez)
        PlayerState = 4,  // server -> client, kanal 1 (guvenilmez)
        MapChanged = 5, // server -> client, kanal 0 (guvenilir) -- harita adini tasir
    };
 
    // Client'in her tick'te gonderdigi girdi. sequence, bu input'u benzersiz
    // sekilde tanimlar; client bunu kendi tahmin gecmisiyle eslestirmek,
    // server ise cevabinda hangi input'u isledigini bildirmek icin kullanir.
    struct PlayerInputPacket {
        uint8_t type = static_cast<uint8_t>(MessageType::PlayerInput);
        uint32_t sequence = 0;
        float deltaTime = 0.0f; // bu input'un temsil ettigi GERCEK sure (saniye)
        PlayerInputCommand cmd;
    };

    // Server'in bir input'u isledikten sonra client'a geri gonderdigi sonuc.
    // sequence, hangi input'un sonucu oldugunu belirtir -- client bunu kendi
    // tahmin gecmisiyle karsilastirip reconciliation yapar.
    struct PlayerStatePacket {
        uint8_t type = static_cast<uint8_t>(MessageType::PlayerState);
        uint32_t sequence = 0;
        PlayerPhysicsState state;
    };
    // Harita ismi sabit uzunlukta bir char dizisinde tasinir (POD, ham bayt
    // olarak gonderilebilmesi icin std::string kullanilmiyor). 63 karakter +
    // null-terminator cogu harita adi icin yeterli.
    struct MapChangedPacket {
        uint8_t type = static_cast<uint8_t>(MessageType::MapChanged);
        char mapName[64] = {};
    };
}

