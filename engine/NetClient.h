#pragma once
#include <string>

// Istemci tarafinin sunucuya olan ag baglantisini yonetir. Su asamada sadece
// baglanti kurulumunu ve basit bir hello/welcome dogrulamasini kapsar; oyun
// verisi (pozisyon, giris komutlari) senkronizasyonu sonraki asamada eklenecektir.
class NetClient {
public:
    static bool Connect(const std::string& hostAddress);
    static void Disconnect();

    // Her karede cagrilmasi gerekir; sunucudan gelen paketleri isler.
    static void Update();

    static bool IsConnected();
};