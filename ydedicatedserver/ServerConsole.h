#pragma once
#include <string>

// Sunucunun kendi konsol penceresinden (stdin) komut okumasini saglar. Okuma
// ayri bir thread'de yapilir, cunku ana dongu enet_host_service ile
// bloklanmaktadir ve ayni thread'de hem ag hem stdin dinlemek mumkun degildir.
// PopCommand, thread-safe sekilde bekleyen bir komutu alir; yoksa false doner.
class ServerConsole {
public:
    static void Start();
    static void Stop();

    // Bekleyen bir komut varsa outLine'a yazar ve true doner; yoksa false doner.
    // Ana dongude her iterasyonda cagrilmasi beklenir.
    static bool PopCommand(std::string& outLine);
};