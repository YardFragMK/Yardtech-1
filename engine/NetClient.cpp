#include "NetClient.h"
#include <enet/enet.h>
#include <NetProtocol.h>
#include <BSPMap.h>
#include <cstring>
#include <deque>
#include "console/Console.h"

namespace {
    ENetHost* s_client = nullptr;
    ENetPeer* s_peer = nullptr;

    uint32_t s_nextSequence = 0;

    bool s_mapChangePending = false;
    std::string s_pendingMapName;

    // Client'in gonderdigi ama henuz sunucu tarafindan onaylanmamis
    // input'lari, gonderim anindaki tahmin edilen durumla birlikte tutar.
    // Reconciliation sirasinda, onaylanan sequence'tan sonraki input'lar
    // buradan alinip tekrar oynatilir.
    struct PendingInput {
        uint32_t sequence;
        float deltaTime;
        PlayerInputCommand cmd;
    };
    std::deque<PendingInput> s_pendingInputs;

    PlayerPhysicsState s_reconciledState;
    bool s_hasReconciledState = false;

    // Reconciliation, sunucudan gelen pozisyonla client'in o input icin
    // sakladigi kendi tahminini karsilastirir. Bu esikten kucuk sapmalar
    // yok sayilir -- surekli kucuk kayan-nokta farklari yuzunden her tick'te
    // gereksiz duzeltme yapmamak icin.
    constexpr float RECONCILE_THRESHOLD = 2.0f;
}

bool NetClient::Connect(const std::string& hostAddress) {
    s_client = enet_host_create(nullptr, 1, NetProtocol::CHANNEL_COUNT, 0, 0);
    if (s_client == nullptr) {
        Console::Log("ENet istemci hostu olusturulamadi.");
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, hostAddress.c_str());
    address.port = NetProtocol::SERVER_PORT;

    s_peer = enet_host_connect(s_client, &address, NetProtocol::CHANNEL_COUNT, 0);
    if (s_peer == nullptr) {
        Console::Log("Sunucuya baglanti girisimi baslatilamadi.");
        enet_host_destroy(s_client);
        s_client = nullptr;
        return false;
    }

    ENetEvent event;
    if (enet_host_service(s_client, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        Console::Log("Sunucuya baglanildi: " + hostAddress);

        uint8_t hello = static_cast<uint8_t>(NetProtocol::MessageType::ClientHello);
        ENetPacket* packet = enet_packet_create(&hello, sizeof(hello), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(s_peer, 0, packet);

        s_nextSequence = 0;
        s_pendingInputs.clear();
        s_hasReconciledState = false;
        return true;
    }

    Console::Log("Sunucuya baglanilamadi (zaman asimi).");
    enet_peer_reset(s_peer);
    s_peer = nullptr;
    enet_host_destroy(s_client);
    s_client = nullptr;
    return false;
}

void NetClient::Disconnect() {
    if (s_peer != nullptr) {
        enet_peer_disconnect(s_peer, 0);
        s_peer = nullptr;
    }
    if (s_client != nullptr) {
        enet_host_destroy(s_client);
        s_client = nullptr;
    }
    s_pendingInputs.clear();
    s_hasReconciledState = false;
    s_mapChangePending = false;
}

void NetClient::Update(float gravity, float jumpforce) {
    if (s_client == nullptr) return;

    ENetEvent event;
    while (enet_host_service(s_client, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_RECEIVE: {
            if (event.packet->dataLength >= 1) {
                uint8_t msgType = event.packet->data[0];

                if (msgType == static_cast<uint8_t>(NetProtocol::MessageType::ServerWelcome)) {
                    Console::Log("Sunucudan welcome mesaji alindi.");
                }
                else if (msgType == static_cast<uint8_t>(NetProtocol::MessageType::PlayerState)
                    && event.packet->dataLength == sizeof(NetProtocol::PlayerStatePacket)) {

                    NetProtocol::PlayerStatePacket statePacket;
                    std::memcpy(&statePacket, event.packet->data, sizeof(statePacket));

                    while (!s_pendingInputs.empty() && s_pendingInputs.front().sequence <= statePacket.sequence) {
                        s_pendingInputs.pop_front();
                    }

                    PlayerPhysicsState replayState = statePacket.state;
                    for (const auto& pending : s_pendingInputs) {
                        SimulatePlayerPhysics(replayState, pending.cmd, g_Map,
                            pending.deltaTime, gravity, jumpforce);
                    }

                    s_reconciledState = replayState;
                    s_hasReconciledState = true;
                }
                else if (msgType == static_cast<uint8_t>(NetProtocol::MessageType::MapChanged)
                    && event.packet->dataLength == sizeof(NetProtocol::MapChangedPacket)) {

                    NetProtocol::MapChangedPacket mapPacket;
                    std::memcpy(&mapPacket, event.packet->data, sizeof(mapPacket));

                    s_pendingMapName = std::string(mapPacket.mapName);
                    s_mapChangePending = true;

                    Console::Log("Sunucu haritayi degistirdi: " + s_pendingMapName);
                }
            }
            enet_packet_destroy(event.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
            Console::Log("Sunucu baglantisi koptu.");
            s_peer = nullptr;
            s_hasReconciledState = false;
            break;
        }
        default:
            break;
        }
    }
}

bool NetClient::IsConnected() {
    return s_peer != nullptr;
}

void NetClient::SendInputCommand(const PlayerInputCommand& cmd, float deltaTime) {
    if (s_peer == nullptr) return;

    uint32_t sequence = s_nextSequence++;

    PendingInput pending;
    pending.sequence = sequence;
    pending.deltaTime = deltaTime;
    pending.cmd = cmd;
    s_pendingInputs.push_back(pending);

    while (s_pendingInputs.size() > 256) {
        s_pendingInputs.pop_front();
    }

    NetProtocol::PlayerInputPacket packet;
    packet.sequence = sequence;
    packet.deltaTime = deltaTime;
    packet.cmd = cmd;

    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(packet), 0);
    enet_peer_send(s_peer, 1, enetPacket);
}

bool NetClient::HasReconciledState() {
    return s_hasReconciledState;
}

const PlayerPhysicsState& NetClient::GetReconciledState() {
    return s_reconciledState;
}

bool NetClient::PollMapChange(std::string& outMapName) {
    if (!s_mapChangePending) return false;
    outMapName = s_pendingMapName;
    s_mapChangePending = false;
    return true;
}