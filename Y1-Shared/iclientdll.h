#pragma once
#include <glm/glm.hpp>
#include "interface.h"
#include "iworldquery.h"
#include "netentity.h"
#include "playerstats.h"
#include "viewsetup.h"
#include "PlayerMovement.h"

// Arayuz degistiginde versiyon artirilir; eski ve yeni modullerin yanlis
// eslesmesi GetInterface'in nullptr donmesiyle yakalanir.
#define CLIENT_DLL_INTERFACE_VERSION "ClientDLL002"
#define ENGINE_CLIENT_INTERFACE_VERSION "EngineClient003"

// client.dll'in engine'den kullandigi hizmetler (exe uygular).
class IEngineClient {
public:
    virtual void GetScreenSize(int* width, int* height) = 0;
    virtual bool IsButtonDown(int buttonCode) = 0;
    virtual void SetMouseCapture(bool capture) = 0;

    // Oyun girdisi (bakis, hareket, atis) su an oyuna mi ait? Konsol, menu veya
    // duraklatma acikken false doner.
    virtual bool IsGameplayInputActive() = 0;

    virtual IWorldQuery* GetWorld() = 0;
    virtual const char* GetLevelName() = 0;
    virtual bool IsInGame() = 0;
    virtual bool IsConnectedToServer() = 0;

    // Engine'in sahip oldugu degiskenleri (orn. nvs_gravity, cm_noclip) okur;
    // bilinmeyen isimde defaultValue doner.
    virtual float GetCvarFloat(const char* name, float defaultValue) = 0;
    virtual int GetCvarInt(const char* name, int defaultValue) = 0;

    virtual void StartListenServer(const char* mapName) = 0;
    virtual void ConnectToServer(const char* address) = 0;
    virtual void Disconnect() = 0;
    virtual void ClientCmd(const char* command) = 0;
    virtual void Quit() = 0;

    // Dosya okuma (VFS): client.dll disk ya da paket bilmez. GetFileSize dosya yoksa -1 doner;
    // ReadFile, tamponu doldurur (tampon dosyadan kucukse false).
    virtual int GetFileSize(const char* vfsPath) = 0;
    virtual bool ReadFile(const char* vfsPath, void* buffer, int bufferSize) = 0;
};

// Engine'in client.dll'den bekledigi giris noktalari (client.dll uygular).
class IBaseClientDLL {
public:
    virtual bool Init(CreateInterfaceFn engineFactory) = 0;
    virtual void Shutdown() = 0;

    virtual void LevelInit(const char* mapName) = 0;
    virtual void LevelShutdown() = 0;

    // Giris olaylari: exe SDL olaylarini cevirip iletir.
    virtual void OnButtonEvent(int buttonCode, bool down) = 0;
    virtual void OnMouseMove(float deltaX, float deltaY, float x, float y) = 0;
    virtual void OnMouseWheel(float deltaY) = 0;
    virtual void OnTextInput(unsigned int codepoint) = 0;

    // Her sabit tick'te once CreateMove ile komut uretilir, ardindan komut
    // RunCommand ile yerel oyuncuya (tahmin/prediction) uygulanir.
    virtual void CreateMove(float frameTime, PlayerInputCommand* outCommand) = 0;
    virtual void RunCommand(const PlayerInputCommand& command, float frameTime) = 0;

    virtual void Update(float frameTime) = 0;
    virtual void GetView(ViewSetup* outView) = 0;

    // Server'dan gelen veriler (tek oyunculuda yerel kopya, MP'de ag uzerinden).
    virtual void ReceiveEntityStates(const NetEntityState* states, int count) = 0;
    virtual void ReceivePlayerState(const PlayerPhysicsState& state, unsigned int lastProcessedCommand) = 0;
    virtual void ReceivePlayerStats(const PlayerStats& stats) = 0;

    // Engine dunyayi cizdikten sonra: entity gorselleri, ardindan HUD/menuler.
    virtual void RenderScene() = 0;
    virtual void RenderUI(int screenWidth, int screenHeight) = 0;
};