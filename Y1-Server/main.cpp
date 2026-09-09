#include <enet/enet.h>
#include <NetProtocol.h>
#include <BSPMap.h>
#include <unordered_map>
#include <cstdio>
#include <cstring>

static constexpr float SERVER_GRAVITY = 900.0f;
static constexpr float SERVER_JUMPFORCE = 250.0f;

int main() {
    printf("Yardtech-1 Dedicated Server alpha 0.02\n");
    printf("======================================\n");
    if (enet_initialize() != 0) {
        printf("WARNING-> ENet baslatilamadi.\n");
        return 1;
    }

    BSPMap serverMap;
    if (!serverMap.Load("nvs1/map/firstmap.bsp")) {
        printf("WARNING-> harita yuklenemedi, collision devre disi kalacak.\n");
    }
    else {
        printf("Harita yuklendi.\n");
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = NetProtocol::SERVER_PORT;

    ENetHost* server = enet_host_create(&address, NetProtocol::MAX_CLIENTS, NetProtocol::CHANNEL_COUNT, 0, 0);
    if (server == nullptr) {
        printf("WARNING-> Sunucu baslatilamadi.\n");
        enet_deinitialize();
        return 1;
    }

    printf("Sunucu %u portunda dinlemeye basladi.\n", NetProtocol::SERVER_PORT);

    std::unordered_map<ENetPeer*, PlayerPhysicsState> playerStates;

    bool running = true;
    while (running) {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                printf("Yeni istemci baglandi.\n");

                PlayerPhysicsState initialState;
                initialState.position = glm::vec3(0.0f, 60.0f, 0.0f);
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

                            // Client'in bildirdigi gercek sureyi kullan, sabit tick
                            // varsaymiyoruz. Asiri/anormal degerlere (lag spike,
                            // ya da kotu niyetli bir istemci) karsi bir tavan
                            // koyuyoruz -- guvenlik acisindan yeterli degil ama
                            // gelistirme asamasinda mantikli bir korunma.
                            float dt = inputPacket.deltaTime;
                            if (dt < 0.0f) dt = 0.0f;
                            if (dt > 0.1f) dt = 0.1f;

                            SimulatePlayerPhysics(
                                it->second, inputPacket.cmd, serverMap,
                                dt, SERVER_GRAVITY, SERVER_JUMPFORCE
                            );

                            NetProtocol::PlayerStatePacket statePacket;
                            statePacket.sequence = inputPacket.sequence;
                            statePacket.state = it->second;
                            ENetPacket* reply = enet_packet_create(&statePacket, sizeof(statePacket), 0);
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