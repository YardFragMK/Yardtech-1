#include "PlayerMovement.h"
#include "BSPMap.h"
#include <cmath>

using namespace PlayerPhysicsConstants;

PlayerPhysicsEvents SimulatePlayerPhysics(
    PlayerPhysicsState& state,
    const PlayerInputCommand& input,
    BSPMap& map,
    float deltaTime,
    float gravity,
    float jumpforce)
{
    PlayerPhysicsEvents events;

    if (input.noclip) {
        state.verticalVelocity = 0.0f;
        state.isCrouching = false;
        return events;
    }

    if (input.consoleOpen) {
        return events;
    }

    // Bu tick'in gercek baslangic noktasi -- hicbir hareket henuz uygulanmadi.
    // Ground check, crouch gecisi ve nihai SlideMove hepsi bu referansa gore
    // calisir; boylece yatay ve dikey hareket TEK bir slide islemiyle birlikte
    // test edilir (duvara capraz yaklasirken de dogru kayma davranisi olusur).
    glm::vec3 tickStartPos = state.position;

    // --- yatay hareket niyetini hesapla (henuz UYGULAMIYORUZ) ---
    glm::vec3 horizontalDelta(0.0f);
    if (input.moveForwardAxis != 0.0f || input.moveRightAxis != 0.0f) {
        glm::vec3 flatForward(cosf(glm::radians(input.yaw)), 0.0f, sinf(glm::radians(input.yaw)));
        flatForward = glm::normalize(flatForward);
        glm::vec3 right = glm::normalize(glm::cross(flatForward, glm::vec3(0.0f, 1.0f, 0.0f)));

        glm::vec3 moveDir = flatForward * input.moveForwardAxis + right * input.moveRightAxis;
        if (glm::length(moveDir) > 0.0001f) {
            moveDir = glm::normalize(moveDir);
        }
        horizontalDelta = moveDir * input.moveSpeed * deltaTime;
    }

    int currentHull = state.isCrouching ? 3 : 1;
    glm::vec3 groundCheckEnd = tickStartPos - glm::vec3(0.0f, 2.0f, 0.0f);
    TraceResult groundTrace = map.TraceLine(tickStartPos, groundCheckEnd, currentHull);
    state.onGround = (groundTrace.fraction < 1.0f);

    if (state.onGround && state.verticalVelocity <= 0.0f) {
        state.verticalVelocity = 0.0f;
    }

    static thread_local bool airborneFromJump = false;
    static thread_local bool airborneFromHardSlam = false;

    if (input.jumpPressed && state.onGround) {
        state.verticalVelocity = jumpforce;
        state.onGround = false;
        airborneFromJump = true;
        airborneFromHardSlam = false;
    }

    if (input.ctrlPressed && !state.onGround && input.canHardSlam) {
        state.verticalVelocity = -jumpforce * HARD_SLAM_VELOCITY_MULTIPLIER;
        airborneFromHardSlam = true;
        airborneFromJump = false;
    }
    else if (state.onGround) {
        if (input.ctrlHeld && !state.isCrouching) {
            // Origin'i asagi kaydiriyoruz ki ayaklar ayni zemin yukseklikginde
            // kalsin. tickStartPos'u da AYNI miktarda kaydirmazsak, asagidaki
            // SlideMove onu yanlis hull-uzayinda yorumlar ve haritanin altina iter.
            state.position.y -= HULL_DIFF;
            tickStartPos.y -= HULL_DIFF;
            state.isCrouching = true;

            const float UNSTICK_STEP = 1.0f;
            const int MAX_UNSTICK_STEPS = static_cast<int>(HULL_DIFF / UNSTICK_STEP) + 2;
            int steps = 0;
            while (map.IsPointSolid(state.position, 3) && steps < MAX_UNSTICK_STEPS) {
                state.position.y += UNSTICK_STEP;
                tickStartPos.y += UNSTICK_STEP;
                steps++;
            }

            if (steps >= MAX_UNSTICK_STEPS) {
                state.position.y += HULL_DIFF;
                tickStartPos.y += HULL_DIFF;
                state.isCrouching = false;
            }
        }
        else if (!input.ctrlHeld && state.isCrouching) {
            glm::vec3 candidateStandPos = state.position + glm::vec3(0.0f, HULL_DIFF, 0.0f);
            if (!map.IsPointSolid(candidateStandPos, 1)) {
                state.position = candidateStandPos;
                tickStartPos.y += HULL_DIFF;
                state.isCrouching = false;
            }
        }
    }

    int moveHull = state.isCrouching ? 3 : 1;

    // Yercekimi entegrasyonu (semi-implicit Euler). Dususte ekstra bir carpan
    // uygulaniyor: yukselirken normal, duserken daha guclu yercekimi.
    float gravityThisFrame = gravity;
    if (state.verticalVelocity < 0.0f) {
        gravityThisFrame *= FALL_GRAVITY_MULTIPLIER;
    }
    state.verticalVelocity -= gravityThisFrame * deltaTime;
    float verticalDelta = state.verticalVelocity * deltaTime;

    // Yatay ve dikey hareketi TEK bir hedef noktada birlestirip TEK bir
    // SlideMove cagrisiyla test ediyoruz. Boylece duvara capraz yaklasirken
    // (yatay+dikey ayni anda) dogru kayma (slide) davranisi olusuyor, ve
    // yatay hareket artik collision'suz gecmiyor.
    glm::vec3 newPos = tickStartPos + horizontalDelta + glm::vec3(0.0f, verticalDelta, 0.0f);

    if (newPos != tickStartPos) {
        glm::vec3 resolvedPos = map.SlideMove(tickStartPos, newPos, moveHull);
        state.position = resolvedPos;

        bool verticalCollisionHappened = std::abs(resolvedPos.y - newPos.y) > 0.01f;
        if (verticalCollisionHappened) {
            if (airborneFromHardSlam) {
                events.hardSlamLanded = true;
            }
            else if (airborneFromJump) {
                events.jumpLanded = true;
            }

            airborneFromJump = false;
            airborneFromHardSlam = false;
            state.verticalVelocity = 0.0f;
        }
    }
    else {
        state.position = tickStartPos;
    }

    return events;
}