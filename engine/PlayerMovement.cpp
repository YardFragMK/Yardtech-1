#include "PlayerMovement.h"
#include "Camera.h"
#include "BSPMap.h"
#include "console/Console.h"
#include "console/CVar.h"
#include "../game/src/Player.h"
#include <cmath>
#include "Engine.h"

void UpdatePlayerPhysics(float deltaTime, glm::vec3 oldPos) {
    if (g_CVar.cm_noclip) {
        // noclip acik: ucus modu, yercekimi/collision/crouch devre disi
        g_Camera.verticalVelocity = 0.0f;
        g_Camera.isCrouching = false;
        return;
    }

    if (Console::IsOpen()) {
        return;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    bool ctrlHeld = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];

    // GoldSrc hull yari-yukseklikleri: hull1 (ayakta) 36, hull3 (egik) 18.
    constexpr float HALF_STAND = 36.0f;
    constexpr float HALF_CROUCH = 18.0f;
    constexpr float HULL_DIFF = HALF_STAND - HALF_CROUCH; // 18

    // --- yer kontrolu ---
    int currentHull = g_Camera.isCrouching ? 3 : 1;
    glm::vec3 groundCheckEnd = oldPos - glm::vec3(0.0f, 2.0f, 0.0f);
    TraceResult groundTrace = g_Map.TraceLine(oldPos, groundCheckEnd, currentHull);
    g_Camera.onGround = (groundTrace.fraction < 1.0f);

    if (g_Camera.onGround && g_Camera.verticalVelocity <= 0.0f) {
        g_Camera.verticalVelocity = 0.0f;
    }

    // --- ziplama ---
    if (keys[SDL_SCANCODE_SPACE] && g_Camera.onGround) {
        g_Camera.verticalVelocity = g_CVar.nvs_jumpforce;
        g_Camera.onGround = false;
    }

    // --- CTRL: havadaysa sert inis, yerdeyse egilme ---
    if (ctrlHeld && !g_Camera.onGround && g_Player.RGDitem) {
        g_Camera.verticalVelocity = -g_CVar.nvs_jumpforce * 4.0f;
    }
    else if (g_Camera.onGround) {
        if (ctrlHeld && !g_Camera.isCrouching) {
            // egilmeye BASLA: origin'i asagi kaydir, ayaklar ayni yukseklikte kalsin.
            // oldPos'u da AYNI miktarda kaydirmazsak, bu frame'in SlideMove'u
            // oldPos'u yanlis hull-uzayinda yorumlar ve haritanin altina iter.
            g_Camera.position.y -= HULL_DIFF;
            oldPos.y -= HULL_DIFF;
            g_Camera.isCrouching = true;
        }
        else if (!ctrlHeld && g_Camera.isCrouching) {
            // kalksaydik nereye giderdik, o NOKTA solid mi diye dogrudan kontrol et
            // (trace degil -- trace'in baslangici hala egik-uzayda olurdu ve hull1 ile tutarsiz olurdu)
            glm::vec3 candidateStandPos = g_Camera.position + glm::vec3(0.0f, HULL_DIFF, 0.0f);
            if (!g_Map.IsPointSolid(candidateStandPos, 1)) {
                g_Camera.position = candidateStandPos;
                oldPos.y += HULL_DIFF;
                g_Camera.isCrouching = false;
            }
            // solid ise kalkamiyoruz, egik kalmaya devam
        }
    }

    int moveHull = g_Camera.isCrouching ? 3 : 1;

    // --- yercekimi entegrasyonu (semi-implicit Euler) ---
    g_Camera.verticalVelocity -= g_CVar.nvs_gravity * deltaTime;
    g_Camera.position.y += g_Camera.verticalVelocity * deltaTime;

    // --- hareketi collision ile coz (duvarda/zeminde kayarak ilerleme) ---
    glm::vec3 newPos = g_Camera.position;
    if (newPos != oldPos) {
        glm::vec3 resolvedPos = g_Map.SlideMove(oldPos, newPos, moveHull);
        g_Camera.position = resolvedPos;

        if (std::abs(resolvedPos.y - newPos.y) > 0.01f) {
            g_Camera.verticalVelocity = 0.0f;
        }
    }
}