#pragma once
#include <string>
#include "PlayerMovement.h"

// Istemcinin sunucuya olan ag baglantisini ve prediction/reconciliation
// icin gereken input gecmisini yonetir. ClientPlayerController, her tick'te
// SendInputCommand'i cagirir; server'dan gelen otoriter durumlar Update()
// icinde islenip reconciliation tetiklenir, sonuc GetReconciledState() ile
// okunur.
class NetClient {
public:
    static bool Connect(const std::string& hostAddress);
    static void Disconnect();

    // Her karede cagrilmasi gerekir; sunucudan gelen paketleri isler ve
    // gerekiyorsa reconciliation yapar.
    static void Update(float gravity, float jumpforce);

    static bool IsConnected();

    // Bu tick'in input'unu sunucuya gonderir ve kendi tahmin gecmisine
    // (localState ile birlikte) kaydeder. localState, bu input uygulandiktan
    // SONRAKI client tahminidir -- cagiran taraf SimulatePlayerPhysics'i
    // cagirdiktan hemen sonra buraya iletmelidir.
   static void SendInputCommand(const PlayerInputCommand& cmd, float deltaTime);

    // Reconciliation sonrasi son bilinen dogru (server tarafindan onaylanmis,
    // henuz onaylanmamis input'lar tekrar oynatilarak guncellenmis) durumu
    // dondurur. ClientPlayerController, kendi tahmini yerine bunu kullanabilir.
    static bool HasReconciledState();
    static const PlayerPhysicsState& GetReconciledState();
};