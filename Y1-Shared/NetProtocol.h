#pragma once
#include <cstdint>
#include <cstddef>

// Istemci ve sunucu arasinda degisecek mesajlarin ortak tanimlarini tutar.
// Her iki taraf da bu sabitleri kullandigi surece ayni protokolde konusur;
// biri degisirse diger tarafin da guncellenmesi gerekir.
namespace NetProtocol {
    constexpr uint16_t SERVER_PORT = 27015;
    constexpr size_t MAX_CLIENTS = 16;
    constexpr size_t CHANNEL_COUNT = 2; // kanal 0: guvenilir/sirali, kanal 1: hizli/guvenilmez

    // Ilk asamada sadece bir bağlanti-dogrulama mesaji tanimlanmistir. Oyuncu
    // girdisi, dunya anlik goruntusu gibi mesaj tipleri ilerleyen asamalarda
    // buraya eklenecektir.
    enum class MessageType : uint8_t {
        ClientHello = 1,
        ServerWelcome = 2,
    };
}