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
    };

    // Client'in her tick'te gonderdigi girdi. PlayerInputCommand zaten POD
    // (sadece sayisal alanlar) oldugu icin dogrudan ham bayt olarak
    // gonderilebilir -- her iki taraf da ayni derleyici/platformda
    // derlendigi surece bu guvenlidir.
    struct PlayerInputPacket {
        uint8_t type = static_cast<uint8_t>(MessageType::PlayerInput);
        PlayerInputCommand cmd;
    };

    // Server'in bir input'u isledikten sonra client'a geri gonderdigi sonuc.
    struct PlayerStatePacket {
        uint8_t type = static_cast<uint8_t>(MessageType::PlayerState);
        PlayerPhysicsState state;
    };
}