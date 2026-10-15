#pragma once
#include <glm/glm.hpp>
#include "interface.h"
#include "iworldquery.h"
#include "netentity.h"
#include "PlayerMovement.h"

#define CLIENT_DLL_INTERFACE_VERSION "ClientDLL001"
#define ENGINE_CLIENT_INTERFACE_VERSION "EngineClient001"

// Engine'in her frame projeksiyon/view kurmak icin client'tan aldigi bilgi.
struct ViewSetup {
    glm::vec3 origin{ 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float fovY = 75.0f;
    float zNear = 0.1f;
    float zFar = 10000.0f;
};

// client.dll'in engine'den kullandigi hizmetler (exe uygular).
class IEngineClient {
public:
    virtual void GetScreenSize(int* width, int* height) = 0;
    virtual bool IsButtonDown(int buttonCode) = 0;
    virtual void SetMouseCapture(bool capture) = 0;

    virtual IWorldQuery* GetWorld() = 0;
    virtual const char* GetLevelName() = 0;
    virtual bool IsInGame() = 0;
    virtual bool IsConnectedToServer() = 0;

    virtual void StartListenServer(const char* mapName) = 0; // yeni oyun
    virtual void ConnectToServer(const char* address) = 0;
    virtual void Disconnect() = 0;
    virtual void ClientCmd(const char* command) = 0;
    virtual void Quit() = 0;
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

    // Her tick'te kullanici komutunu uretir ve engine bunu server'a iletir.
    virtual void CreateMove(float frameTime, PlayerInputCommand* outCommand) = 0;

    virtual void Update(float frameTime) = 0;
    virtual void GetView(ViewSetup* outView) = 0;

    // Server'dan gelen entity durumlari (SP'de yerel kopya, MP'de ag uzerinden).
    virtual void ReceiveEntityStates(const NetEntityState* states, int count) = 0;
    virtual void ReceivePlayerState(const PlayerPhysicsState& state, unsigned int lastProcessedCommand) = 0;

    // Engine dunyayi cizdikten sonra: entity gorselleri, ardindan HUD/menuler.
    virtual void RenderScene() = 0;
    virtual void RenderUI(int screenWidth, int screenHeight) = 0;
};