#include <enet/enet.h>
#include <NetProtocol.h>
#include <BSPMap.h>
#include "ServerConsole.h"
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <sstream>

static constexpr float SERVER_GRAVITY = 900.0f;
static constexpr float SERVER_JUMPFORCE = 250.0f;

static BSPMap g_serverMap;
static ENetHost* g_server = nullptr;
static std::unordered_map<ENetPeer*, PlayerPhysicsState> g_playerStates;
static size_t g_maxClients = NetProtocol::MAX_CLIENTS;

// Sunucu operatorunun konsoldan girdigi komutlari isler. Bu komutlar hicbir
// zaman client'lara gonderilmez -- yalnizca sunucu tarafinda, dedicated
// server uygulamasinin kendi terminalinden calisir.
static void ExecuteServerCommand(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "server_changemap") {
        std::string mapName;
        if (iss >> mapName) {
            std::string path = "nvs1/map/" + mapName + ".bsp";
            if (g_serverMap.Load(path)) {
                printf("Harita degistirildi: %s\n", mapName.c_str());

                for (auto& pair : g_playerStates) {
                    pair.second = PlayerPhysicsState{};
                    pair.second.position = glm::vec3(0.0f, 60.0f, 0.0f);
                }

                // Tum bagli client'lara yeni haritanin adini bildir --
                // guvenilir kanaldan (0) gonderiyoruz, bu paketin kaybolmasi
                // client'in eski haritada takili kalmasina yol acar.
                NetProtocol::MapChangedPacket mapPacket;
                strncpy_s(mapPacket.mapName, mapName.c_str(), sizeof(mapPacket.mapName) - 1);

                for (auto& pair : g_playerStates) {
                    ENetPacket* packet = enet_packet_create(&mapPacket, sizeof(mapPacket), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(pair.first, 0, packet);
                }
            }
            else {
                printf("ERROR-> harita yuklenemedi: %s\n", mapName.c_str());
            }
        }
        else {
            printf("Kullanim: server_changemap <mapname>\n");
        }
    }
    else if (cmd == "server_changemaxclientnumber") {
        size_t value;
        if (iss >> value) {
            g_maxClients = value;
            printf("Maksimum istemci sayisi %zu olarak ayarlandi (mevcut baglantilari etkilemez, sadece yeni baglantilar icin gecerlidir).\n", value);
        }
        else {
            printf("Kullanim: server_changemaxclientnumber <number>\n");
        }
    }
    else if (cmd == "server_status") {
        printf("Bagli istemci sayisi: %zu / %zu\n", g_playerStates.size(), g_maxClients);
    }
    else if (!cmd.empty()) {
        printf("Bilinmeyen komut: %s\n", cmd.c_str());
    }
}

int main() {
    printf("Yardtech-1 Dedicated Server alpha 0.03\n");
    printf("======================================\n");
    printf("Komutlar: server_changemap <mapname>, server_changemaxclientnumber <number>, server_status\n");

    if (enet_initialize() != 0) {
        printf("WARNING-> ENet baslatilamadi.\n");
        return 1;
    }

    if (!g_serverMap.Load("nvs1/map/firstmap.bsp")) {
        printf("WARNING-> harita yuklenemedi, collision devre disi kalacak.\n");
    }
    else {
        printf("Harita yuklendi.\n");
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = NetProtocol::SERVER_PORT;

    g_server = enet_host_create(&address, NetProtocol::MAX_CLIENTS, NetProtocol::CHANNEL_COUNT, 0, 0);
    if (g_server == nullptr) {
        printf("WARNING-> Sunucu baslatilamadi.\n");
        enet_deinitialize();
        return 1;
    }

    printf("Sunucu %u portunda dinlemeye basladi.\n", NetProtocol::SERVER_PORT);

    ServerConsole::Start();

    bool running = true;
    while (running) {
        // Konsoldan gelen bekleyen komutlari isle.
        std::string line;
        while (ServerConsole::PopCommand(line)) {
            ExecuteServerCommand(line);
        }

        ENetEvent event;
        while (enet_host_service(g_server, &event, 50) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                if (g_playerStates.size() >= g_maxClients) {
                    printf("Baglanti reddedildi (maksimum istemci sayisina ulasildi).\n");
                    enet_peer_disconnect(event.peer, 0);
                    break;
                }

                printf("Yeni istemci baglandi.\n");

                PlayerPhysicsState initialState;
                initialState.position = glm::vec3(0.0f, 60.0f, 0.0f);
                g_playerStates[event.peer] = initialState;
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

                        auto it = g_playerStates.find(event.peer);
                        if (it != g_playerStates.end()) {
                            NetProtocol::PlayerInputPacket inputPacket;
                            std::memcpy(&inputPacket, event.packet->data, sizeof(inputPacket));

                            float dt = inputPacket.deltaTime;
                            if (dt < 0.0f) dt = 0.0f;
                            if (dt > 0.1f) dt = 0.1f;

                            SimulatePlayerPhysics(
                                it->second, inputPacket.cmd, g_serverMap,
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
                g_playerStates.erase(event.peer);
                break;
            }
            default:
                break;
            }
        }
    }

    ServerConsole::Stop();
    enet_host_destroy(g_server);
    enet_deinitialize();
    return 0;
}