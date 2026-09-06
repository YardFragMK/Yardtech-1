#include "NetClient.h"
#include <enet/enet.h>
#include <NetProtocol.h>
#include "console/Console.h"

namespace {
    ENetHost* s_client = nullptr;
    ENetPeer* s_peer = nullptr;
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
}

void NetClient::Update() {
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
            }
            enet_packet_destroy(event.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
            Console::Log("Sunucu baglantisi koptu.");
            s_peer = nullptr;
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