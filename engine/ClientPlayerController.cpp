#include "ClientPlayerController.h"
#include "PlayerMovement.h" // Y1-Shared
#include "Camera.h"
#include "BSPMap.h"
#include "console/Console.h"
#include "console/CVar.h"
#include "../game/src/Player.h"
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
    cmd.consoleOpen = false; // yukarida zaten erken cikildi, ama acikca belirtiyoruz
    cmd.yaw = g_Camera.yaw;
    cmd.moveSpeed = g_Camera.moveSpeed;
    cmd.canHardSlam = g_Player.RGDitem;

    if (keys[SDL_SCANCODE_W]) cmd.moveForwardAxis += 1.0f;
    if (keys[SDL_SCANCODE_S]) cmd.moveForwardAxis -= 1.0f;
    if (keys[SDL_SCANCODE_D]) cmd.moveRightAxis += 1.0f;
    if (keys[SDL_SCANCODE_A]) cmd.moveRightAxis -= 1.0f;

    // Roll efekti tamamen gorsel/client'a ozgu -- yatay hareket ekseni
    // pozitifse (D/sag) kameray saga, negatifse (A/sol) sola yatiriyoruz.
    // noclip'te bu efekti kapatiyoruz, tipik olarak ucus modunda roll istenmez.
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

    // --- Saf fizigi CAGIR: bu kisim Y1-Shared'daki ortak koddan geliyor ---
    PlayerPhysicsState state;
    state.position = g_Camera.position;
    state.verticalVelocity = g_Camera.verticalVelocity;
    state.onGround = g_Camera.onGround;
    state.isCrouching = g_Camera.isCrouching;

    PlayerPhysicsEvents events = SimulatePlayerPhysics(
        state, cmd, g_Map, deltaTime, g_CVar.nvs_gravity, g_CVar.nvs_jumpforce
    );

    // --- Sonucu gorsel global'lere YAZ: yine client'a ozgu ---
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

    float horizDist = glm::length(glm::vec2(g_Camera.position.x - oldPos.x, g_Camera.position.z - oldPos.z));
    float horizSpeed = (deltaTime > 0.0001f) ? (horizDist / deltaTime) : 0.0f;
    g_Camera.UpdateViewBob(deltaTime, horizSpeed, g_Camera.onGround);
}