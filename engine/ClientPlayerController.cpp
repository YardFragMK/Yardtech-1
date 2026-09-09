#include "ClientPlayerController.h"
#include "PlayerMovement.h" // Y1-Shared
#include "Camera.h"
#include "BSPMap.h"
#include "console/Console.h"
#include "console/CVar.h"
#include "../game/src/Player.h"
#include "NetClient.h"
#include <SDL.h>

void UpdatePlayerPhysics(float deltaTime, glm::vec3 oldPos) {
    g_Player.UpdateWeapons(deltaTime);

    if (Console::IsOpen()) {
        return;
    }

    // --- Input'u OKU: bu kisim tamamen client'a ozgu ---
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    static bool spaceHeldPrevFrame = false;
    static bool ctrlHeldPrevFrame = false;
    bool spaceHeldNow = keys[SDL_SCANCODE_SPACE];
    bool ctrlHeldNow = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];

    PlayerInputCommand cmd;
    cmd.jumpPressed = spaceHeldNow && !spaceHeldPrevFrame;
    cmd.ctrlPressed = ctrlHeldNow && !ctrlHeldPrevFrame;
    cmd.ctrlHeld = ctrlHeldNow;
    cmd.noclip = g_CVar.cm_noclip;
    cmd.consoleOpen = false;
    cmd.yaw = g_Camera.yaw;
    cmd.moveSpeed = g_Camera.moveSpeed;
    cmd.canHardSlam = g_Player.RGDitem;

    if (keys[SDL_SCANCODE_W]) cmd.moveForwardAxis += 1.0f;
    if (keys[SDL_SCANCODE_S]) cmd.moveForwardAxis -= 1.0f;
    if (keys[SDL_SCANCODE_D]) cmd.moveRightAxis += 1.0f;
    if (keys[SDL_SCANCODE_A]) cmd.moveRightAxis -= 1.0f;

    if (!g_CVar.cm_noclip) {
        if (cmd.moveRightAxis > 0.0f) {
            g_Camera.targetRoll = g_Camera.maxRoll;
        }
        else if (cmd.moveRightAxis < 0.0f) {
            g_Camera.targetRoll = -g_Camera.maxRoll;
        }
    }

    spaceHeldPrevFrame = spaceHeldNow;
    ctrlHeldPrevFrame = ctrlHeldNow;

    bool multiplayer = NetClient::IsConnected();

    if (multiplayer) {
        // Sunucudan gelen onaylanmis/reconcile edilmis durumu once uygula --
        // boylece bu tick'in tahmini, sunucuyla en son senkron olan noktadan
        // baslar. NetClient::Update zaten reconciliation'i (onaylanmamis
        // input'lari tekrar oynatarak) tamamlamis durumda sunar.
        NetClient::Update(g_CVar.nvs_gravity, g_CVar.nvs_jumpforce);

        if (NetClient::HasReconciledState()) {
            const PlayerPhysicsState& reconciled = NetClient::GetReconciledState();
            g_Camera.position = reconciled.position;
            g_Camera.verticalVelocity = reconciled.verticalVelocity;
            g_Camera.onGround = reconciled.onGround;
            g_Camera.isCrouching = reconciled.isCrouching;
        }
    }

    // --- Saf fizigi CAGIR: bu tick'in kendi input'unu simule et (prediction) ---
    PlayerPhysicsState state;
    state.position = g_Camera.position;
    state.verticalVelocity = g_Camera.verticalVelocity;
    state.onGround = g_Camera.onGround;
    state.isCrouching = g_Camera.isCrouching;

    PlayerPhysicsEvents events = SimulatePlayerPhysics(
        state, cmd, g_Map, deltaTime, g_CVar.nvs_gravity, g_CVar.nvs_jumpforce
    );

    g_Camera.position = state.position;
    g_Camera.verticalVelocity = state.verticalVelocity;
    g_Camera.onGround = state.onGround;
    g_Camera.isCrouching = state.isCrouching;

    if (events.hardSlamLanded) {
        g_Camera.TriggerLandingShake(22.0f);
    }
    else if (events.jumpLanded) {
        g_Camera.TriggerLandingShake(6.0f);
    }

    if (multiplayer) {
        NetClient::SendInputCommand(cmd, deltaTime);
    }

    float horizDist = glm::length(glm::vec2(g_Camera.position.x - oldPos.x, g_Camera.position.z - oldPos.z));
    float horizSpeed = (deltaTime > 0.0001f) ? (horizDist / deltaTime) : 0.0f;
    g_Camera.UpdateViewBob(deltaTime, horizSpeed, g_Camera.onGround);
}