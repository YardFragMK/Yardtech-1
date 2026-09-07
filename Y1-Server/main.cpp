#include <enet/enet.h>
#include <NetProtocol.h>
#include <BSPMap.h>
#include <unordered_map>
#include <cstdio>

// Bu asamada sunucu, her oyuncunun input'unu ALDIGI ANDA fizigi kosturup
// sonucu geri gonderir -- gercek bir sabit-tick sunucu dongusunden (input
// buffer'lama, ayrik simulasyon adimlari) farkli olarak, simulasyon input
// varisina bagli calisir. Client 60Hz'de input gonderdigi surece pratikte
// dogru sonuc verir, ama gercek bir production sunucusunda input'un gec
// gelmesi/kaybolmasi durumlarini ayrica ele almak gerekir -- bu, ilerleyen
// bir asamada eklenecektir.

static constexpr float SERVER_GRAVITY = 900.0f;
static constexpr float SERVER_JUMPFORCE = 250.0f;

int main() {
    if (enet_initialize() != 0) {
        printf("ENet baslatilamadi.\n");
        return 1;
    }

    BSPMap serverMap;
    if (!serverMap.Load("nvs1/map/firstmap.bsp")) {
        printf("UYARI: harita yuklenemedi, collision devre disi kalacak.\n");
    }
    else {
        printf("Harita yuklendi.\n");
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

    // Her bagli oyuncunun fizik durumu, o oyuncuyu temsil eden ENetPeer
    // pointer'ina gore tutulur. Peer, baglanti suresince sabit ve benzersiz
    // kaldigi icin anahtar olarak kullanilabilir.
    std::unordered_map<ENetPeer*, PlayerPhysicsState> playerStates;

    bool running = true;
    while (running) {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                printf("Yeni istemci baglandi.\n");

                PlayerPhysicsState initialState;
                initialState.position = glm::vec3(0.0f, 60.0f, 0.0f); // gecici sabit spawn noktasi
                playerStates[event.peer] = initialState;
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
                    else if (msgType == static_cast<uint8_t>(NetProtocol::MessageType::PlayerInput)
                        && event.packet->dataLength == sizeof(NetProtocol::PlayerInputPacket)) {

                        auto it = playerStates.find(event.peer);
                        if (it != playerStates.end()) {
                            NetProtocol::PlayerInputPacket inputPacket;
                            std::memcpy(&inputPacket, event.packet->data, sizeof(inputPacket));

                            SimulatePlayerPhysics(
                                it->second, inputPacket.cmd, serverMap,
                                NetProtocol::FIXED_DELTA_TIME, SERVER_GRAVITY, SERVER_JUMPFORCE
                            );

                            NetProtocol::PlayerStatePacket statePacket;
                            statePacket.state = it->second;
                            ENetPacket* reply = enet_packet_create(&statePacket, sizeof(statePacket), 0); // guvenilmez, hizli
                            enet_peer_send(event.peer, 1, reply);
                        }
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                printf("Bir istemcinin baglantisi koptu.\n");
                playerStates.erase(event.peer);
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