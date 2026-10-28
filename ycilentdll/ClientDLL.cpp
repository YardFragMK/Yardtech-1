#include "pch.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "iclientdll.h"
#include "ienginecommon.h"
#include "ienginesound.h"
#include "buttoncodes.h"
#include "ClientCamera.h"
#include "MapSounds.h"

namespace {
    // BSP "origin" metnini ("x y z") motor uzayina (Y-up) cevirir; BSP'nin
    // Z-up eksenlerinden (x, z, -y) donusumu uygulanir.
    glm::vec3 ParseBspOrigin(const char* text) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        std::istringstream stream(text);
        stream >> x >> y >> z;
        return glm::vec3(x, z, -y);
    }
    IEngineSound* s_sound = nullptr;
    IEngineConsole* s_commandConsole = nullptr;
    const ClientCamera* s_commandCamera = nullptr;

    // snd_play <yol>: konumsuz (2B) ses calar. Ornek: snd_play sound/test.wav
    void CmdSoundPlay(int argc, const char* const* argv) {
        if (s_sound == nullptr || s_commandConsole == nullptr) return;
        if (argc < 2) {
            s_commandConsole->Print("Usage: snd_play <ses dosyasi yolu>");
            return;
        }
        if (s_sound->Play2D(argv[1], 1.0f, SOUND_CHANNEL_SFX, false) == 0) {
            s_commandConsole->Print((std::string("Ses calinamadi: ") + argv[1]).c_str());
        }
    }

    // snd_play3d <yol>: sesi oyuncunun 300 birim sagina koyar; panlama ve sonumleme test edilir.
    void CmdSoundPlay3D(int argc, const char* const* argv) {
        if (s_sound == nullptr || s_commandConsole == nullptr || s_commandCamera == nullptr) return;
        if (argc < 2) {
            s_commandConsole->Print("Usage: snd_play3d <ses dosyasi yolu>");
            return;
        }

        const glm::vec3 position = s_commandCamera->GetEyePosition() + s_commandCamera->Right() * 300.0f;
        if (s_sound->Play3D(argv[1], position.x, position.y, position.z, 1.0f, false) == 0) {
            s_commandConsole->Print((std::string("Ses calinamadi: ") + argv[1]).c_str());
        }
    }
}

// client.dll'in gercek gerceklestirmesi. Kamera, girdi (komut uretimi), yerel
// oyuncu tahmini (prediction) ve client cvar'lari burada yasar. Engine'e ve
// diger modullere yalnizca arayuzlerle baglidir.
class ClientDLL : public IBaseClientDLL {
public:
    bool Init(CreateInterfaceFn engineFactory) override {
        m_console = GetInterface<IEngineConsole>(engineFactory, ENGINE_CONSOLE_INTERFACE_VERSION);
        m_engine = GetInterface<IEngineClient>(engineFactory, ENGINE_CLIENT_INTERFACE_VERSION);

        if (m_console == nullptr) return false;
        if (m_engine == nullptr) {
            Log("client.dll: IEngineClient saglanamadi");
            return false;
        }

        // Ses hizmeti isteğe bağlıdır; yoksa oyun sessiz çalışmaya devam eder.
        m_sound = GetInterface<IEngineSound>(engineFactory, ENGINE_SOUND_INTERFACE_VERSION);
        s_sound = m_sound;
        s_commandConsole = m_console;
        s_commandCamera = &m_camera;
        m_console->RegisterCommand("snd_play", CmdSoundPlay, "Konumsuz ses calar: snd_play <yol>", CVAR_NONE);
        m_console->RegisterCommand("snd_play3d", CmdSoundPlay3D, "Oyuncunun sagindan 3B ses calar: snd_play3d <yol>", CVAR_NONE);
        m_cmSpeed = m_console->RegisterFloat("cm_speed", 320.0f, "Oyuncu hareket hizi", CVAR_DEVELOPER | CVAR_SINGLEPLAYER_ONLY);
        m_sensitivity = m_console->RegisterFloat("cm_sensitivity", 0.1f, "Fare hassasiyeti", CVAR_ARCHIVE);
        m_rollMax = m_console->RegisterFloat("cm_rollmax", 7.0f, "Yanal harekette en buyuk kamera yatmasi (derece)", CVAR_DEVELOPER);
        m_rollSpeed = m_console->RegisterFloat("cm_rollspeed", 8.0f, "Kamera yatma hizi", CVAR_DEVELOPER);
        m_bobStyle = m_console->RegisterInt("cm_viewbobstyle", 1, "Yurume sallanma stili (0 yumusak, 1 Doom)", CVAR_ARCHIVE);

        if (m_cmSpeed == nullptr || m_sensitivity == nullptr || m_rollMax == nullptr ||
            m_rollSpeed == nullptr || m_bobStyle == nullptr) {
            Log("client.dll: cvar kaydi basarisiz");
            return false;
        }
        Log("client.dll: Init basarili, cvar'lar kaydedildi");
        return true;
    }

    void Shutdown() override {
        Log("client.dll: Shutdown");
        MapSounds::Shutdown();
        m_world = nullptr;
        m_engine = nullptr;
        m_console = nullptr;
        if (m_sound != nullptr) m_sound->StopAll();
        m_sound = nullptr;
        s_sound = nullptr;
        s_commandConsole = nullptr;
        s_commandCamera = nullptr;
    }

    void LevelInit(const char* mapName) override {
        m_world = m_engine->GetWorld();
        m_state = PlayerPhysicsState();
        m_pendingActions = PlayerActionCommand();
        m_entities.clear();

        glm::vec3 spawn(0.0f, 60.0f, 0.0f);
        float spawnYaw = -90.0f;
        bool foundStart = false;

        if (m_world != nullptr) {
            const int count = m_world->GetEntityCount();
            for (int i = 0; i < count; i++) {
                const char* classname = m_world->GetEntityKeyValue(i, "classname");
                if (classname == nullptr || std::strcmp(classname, "player_start") != 0) continue;

                if (const char* origin = m_world->GetEntityKeyValue(i, "origin")) {
                    spawn = ParseBspOrigin(origin);
                    spawn.y += 36.0f; // origin ayak hizasinda; goz/govde hizasina tasi
                    foundStart = true;
                }
                if (const char* angle = m_world->GetEntityKeyValue(i, "angle")) {
                    spawnYaw = static_cast<float>(std::atof(angle));
                }
                break;
            }
        }

        m_state.position = spawn;
        m_camera.Reset(spawn, spawnYaw);
        // Haritadaki tum ses entity'lerini (ambient_generic, env_sound, speaker, cdaudio, tetikleyiciler) baslat.
        MapSounds::Init(m_engine, m_sound, m_console);

        Log((std::string("client.dll: LevelInit ") + (mapName ? mapName : "") +
            (foundStart ? " (player_start bulundu)" : " (player_start bulunamadi, varsayilan konum)")).c_str());
    }

    void LevelShutdown() override {
        MapSounds::Shutdown();
        if (m_sound != nullptr) m_sound->StopAll();
        m_world = nullptr;
        m_entities.clear();
        Log("client.dll: LevelShutdown");
    }

    void OnButtonEvent(int buttonCode, bool down) override {
        if (!down || !InputActive()) return;

        // 1..9 -> silah 0..8, 0 -> silah 9
        if (buttonCode >= '1' && buttonCode <= '9') {
            m_pendingActions.switchToWeapon = buttonCode - '1';
            return;
        }
        if (buttonCode == '0') {
            m_pendingActions.switchToWeapon = LASTGUN;
            return;
        }

        switch (buttonCode) {
        case BUTTON_MOUSE_LEFT:  m_pendingActions.attack1Count++;
            break;
        case BUTTON_MOUSE_RIGHT: m_pendingActions.attack2Count++; break;
        case 'R': m_pendingActions.reloadCount++; break;
        default: break;
        }
    }

    void OnMouseMove(float deltaX, float deltaY, float, float) override {
        if (!InputActive()) return;
        m_camera.ProcessMouseMovement(deltaX, deltaY, *m_sensitivity);
    }

    void OnMouseWheel(float) override {}
    void OnTextInput(unsigned int) override {}

    void CreateMove(float, PlayerInputCommand* outCommand) override {
        if (outCommand == nullptr) return;

        const bool active = InputActive();

        PlayerInputCommand cmd;
        cmd.consoleOpen = !active; // girdi oyuna ait degilse fizik dondurulur
        cmd.yaw = m_camera.yaw;
        cmd.pitch = m_camera.pitch;
        cmd.moveSpeed = *m_cmSpeed;
        cmd.canHardSlam = true;
        cmd.noclip = m_engine->GetCvarInt("cm_noclip", 0) != 0;

        bool spaceNow = false;
        bool ctrlNow = false;

        if (active) {
            spaceNow = m_engine->IsButtonDown(BUTTON_SPACE);
            ctrlNow = m_engine->IsButtonDown(BUTTON_LCTRL);

            if (m_engine->IsButtonDown('W')) cmd.moveForwardAxis += 1.0f;
            if (m_engine->IsButtonDown('S')) cmd.moveForwardAxis -= 1.0f;
            if (m_engine->IsButtonDown('D')) cmd.moveRightAxis += 1.0f;
            if (m_engine->IsButtonDown('A')) cmd.moveRightAxis -= 1.0f;

            cmd.actions = m_pendingActions;
        }

        // Girdi oyuna ait degilken biriken eylemler atilir.
        m_pendingActions = PlayerActionCommand();

        // Zıplama ve sert inis tuşa yeni basildiginda tetiklenir (yukselen kenar);
        // tus basili tutulursa tekrar etmez.
        cmd.jumpPressed = spaceNow && !m_spaceWasDown;
        cmd.ctrlPressed = ctrlNow && !m_ctrlWasDown;
        cmd.ctrlHeld = ctrlNow;
        m_spaceWasDown = spaceNow;
        m_ctrlWasDown = ctrlNow;

        cmd.actions.aimYaw = m_camera.yaw;
        cmd.actions.aimPitch = m_camera.pitch;
        cmd.actions.attack1Held = active && m_engine->IsButtonDown(BUTTON_MOUSE_LEFT);
        cmd.actions.attack2Held = active && m_engine->IsButtonDown(BUTTON_MOUSE_RIGHT);

        *outCommand = cmd;
    }

    void RunCommand(const PlayerInputCommand& cmd, float frameTime) override {
        m_camera.Update(frameTime, *m_rollSpeed);

        if (cmd.consoleOpen || m_world == nullptr) return;

        const glm::vec3 oldPosition = m_state.position;

        if (!cmd.noclip) {
            m_camera.SetRollInput(cmd.moveRightAxis, *m_rollMax);
        }

        const float gravity = m_engine->GetCvarFloat("nvs_gravity", 900.0f);
        const float jumpForce = m_engine->GetCvarFloat("nvs_jumpforce", 250.0f);

        PlayerPhysicsEvents events = SimulatePlayerPhysics(m_state, cmd, *m_world, frameTime, gravity, jumpForce);

        m_camera.position = m_state.position;
        m_camera.isCrouching = m_state.isCrouching;

        if (events.hardSlamLanded) {
            m_camera.TriggerLandingShake(22.0f);
        }
        else if (events.jumpLanded) {
            m_camera.TriggerLandingShake(6.0f);
        }

        const float horizontalDistance = glm::length(
            glm::vec2(m_state.position.x - oldPosition.x, m_state.position.z - oldPosition.z));
        const float horizontalSpeed = (frameTime > 0.0001f) ? (horizontalDistance / frameTime) : 0.0f;
        m_camera.UpdateViewBob(frameTime, horizontalSpeed, m_state.onGround, *m_bobStyle);
    }

    void Update(float frameTime) override {
        if (m_sound == nullptr) return;

        const glm::vec3 eye = m_camera.GetEyePosition();
        const glm::vec3 forward = m_camera.Forward();
        m_sound->SetListener(eye.x, eye.y, eye.z, forward.x, forward.y, forward.z);

        // m_state.position: oyuncunun fizik merkezi (uyelerinin adi farkliysa uyarla).
        MapSounds::Update(frameTime, m_state.position, eye);
    }

    void GetView(ViewSetup* outView) override {
        m_camera.FillView(outView);
        if (outView != nullptr) outView->fovY += m_stats.fovBoost;
    }

    void ReceiveEntityStates(const NetEntityState* states, int count) override {
        if (states == nullptr || count <= 0) {
            m_entities.clear();
            return;
        }
        m_entities.assign(states, states + count);
    }

    void ReceivePlayerState(const PlayerPhysicsState& state, unsigned int) override {
        // Sunucu onayli (reconcile edilmis) durum: bayraklar dahil tum state alinir.
        m_state = state;
        m_camera.position = state.position;
        m_camera.isCrouching = state.isCrouching;
    }

    void ReceivePlayerStats(const PlayerStats& stats) override {
        m_stats = stats;
    }

    void RenderScene() override {}
    void RenderUI(int, int) override {}

private:
    bool InputActive() const {
        return m_engine != nullptr && m_engine->IsGameplayInputActive();
    }

    void Log(const char* message) const {
        if (m_console != nullptr) m_console->Print(message);
    }

    IEngineConsole* m_console = nullptr;
    IEngineClient* m_engine = nullptr;
    IEngineSound* m_sound = nullptr;
    IWorldQuery* m_world = nullptr;

    float* m_cmSpeed = nullptr;
    float* m_sensitivity = nullptr;
    float* m_rollMax = nullptr;
    float* m_rollSpeed = nullptr;
    int* m_bobStyle = nullptr;

    ClientCamera m_camera;
    PlayerPhysicsState m_state;
    PlayerActionCommand m_pendingActions;
    PlayerStats m_stats;
    std::vector<NetEntityState> m_entities;

    bool m_spaceWasDown = false;
    bool m_ctrlWasDown = false;
};

EXPOSE_SINGLE_INTERFACE(ClientDLL, IBaseClientDLL, CLIENT_DLL_INTERFACE_VERSION);