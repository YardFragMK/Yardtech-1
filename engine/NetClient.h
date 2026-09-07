#pragma once
#include <string>
#include "PlayerMovement.h"

// Istemci tarafinin sunucuya olan ag baglantisini yonetir. Su asamada
// baglanti kurulumu, hello/welcome dogrulamasi, ve tek yonlu input->state
// akisini kapsar. Client-side prediction/reconciliation henuz eklenmedi --
// server'dan gelen pozisyon su an sadece gozlemlenip loglanmaktadir,
// kameranin kontrolune henuz baglanmamistir.
class NetClient {
public:
    static bool Connect(const std::string& hostAddress);
    static void Disconnect();

    // Her karede cagrilmasi gerekir; sunucudan gelen paketleri isler.
    static void Update();

    static bool IsConnected();

    // Bu tick'in input'unu sunucuya gonderir. Baglanti yoksa sessizce hicbir
    // sey yapmaz.
    static void SendInputCommand(const PlayerInputCommand& cmd);

    // Sunucudan en son alinan otoriter durum. hasState false ise henuz hic
    // durum alinmamis demektir.
    static const PlayerPhysicsState& GetLastServerState();
    static bool HasServerState();
};