#include <enet/enet.h>
#include <NetProtocol.h>
#include <cstdio>

// Basit bir dedicated server dongusu: belirlenen portta dinler, baglanan
// istemcilerin "hello" mesajina bir "welcome" mesajiyla karsilik verir.
// Bu asamada henuz oyun durumu (harita, oyuncu pozisyonlari) yonetilmez;
// amac sadece baglanti katmaninin dogru calistigini dogrulamaktir.
int main() {
    printf("Yardtech-1 Dedicated Server alpha 0.01\n");
    printf("======================================\n");
    if (enet_initialize() != 0) {
        printf("ENet baslatilamadi.\n");
        return 1;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = NetProtocol::SERVER_PORT;

    ENetHost* server = enet_host_create(&address, NetProtocol::MAX_CLIENTS, NetProtocol::CHANNEL_COUNT, 0, 0);
    if (server == nullptr) {
        printf("Sunucu baslatilamadi.\n");
        enet_deinitialize();
        return 1;
    }

    printf("Sunucu %u portunda dinlemeye basladi.\n", NetProtocol::SERVER_PORT);

    bool running = true;
    while (running) {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                printf("Yeni istemci baglandi (port: %u).\n", event.peer->address.port);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength >= 1) {
                    uint8_t msgType = event.packet->data[0];
                    if (msgType == static_cast<uint8_t>(NetProtocol::MessageType::ClientHello)) {
                        printf("Istemciden hello mesaji alindi, welcome ile yanitlaniyor.\n");

                        uint8_t reply = static_cast<uint8_t>(NetProtocol::MessageType::ServerWelcome);
                        ENetPacket* packet = enet_packet_create(&reply, sizeof(reply), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                printf("Bir istemcinin baglantisi koptu.\n");
                break;
            }
            default:
                break;
            }
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}