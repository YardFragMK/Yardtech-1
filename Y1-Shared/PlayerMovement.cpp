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
    // calisir; boylece yatay ve dikey hareket tek bir slide islemiyle birlikte
    // test edilir.
    glm::vec3 tickStartPos = state.position;

    // Yatay hareket niyeti hesaplanir, henuz uygulanmaz.
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

    // Oyuncunun havaya nasil ciktigi (ziplayarak mi, sert inisle mi) artik
    // state icinde, oyuncuya ozel olarak tutuluyor.
    if (input.jumpPressed && state.onGround) {
        state.verticalVelocity = jumpforce;
        state.onGround = false;
        state.airborneFromJump = true;
        state.airborneFromHardSlam = false;
    }

    if (input.ctrlPressed && !state.onGround && input.canHardSlam) {
        state.verticalVelocity = -jumpforce * HARD_SLAM_VELOCITY_MULTIPLIER;
        state.airborneFromHardSlam = true;
        state.airborneFromJump = false;
    }
    else if (state.onGround) {
        if (input.ctrlHeld && !state.isCrouching) {
            // Origin'i asagi kaydiriyoruz ki ayaklar ayni zemin yukseklikginde
            // kalsin. tickStartPos'u da ayni miktarda kaydirmazsa, asagidaki
            // SlideMove onu yanlis hull-uzayinda yorumlar ve haritanin altina iter.
            state.position.y -= HULL_DIFF;
            tickStartPos.y -= HULL_DIFF;
            state.isCrouching = true;

            // Duz zeminde bu kaydirma guvenlidir, ama egimli bir yuzeyde hull3'un
            // gercek sinir noktasi sadece dikey degil, egim normali dogrultusunda
            // kayar. Bu da yeni origin'in yuzeyin icine gomulmesine yol acabilir --
            // gomulu origin sonraki tick'lerde collision tarafindan yakalanamayip
            // oyuncunun haritadan dusmesine sebep olur. Kucuk adimlarla yukari
            // iterek gomulmeyi duzeltiyoruz.
            const float UNSTICK_STEP = 1.0f;
            const int MAX_UNSTICK_STEPS = static_cast<int>(HULL_DIFF / UNSTICK_STEP) + 2;
            int steps = 0;
            while (map.IsPointSolid(state.position, 3) && steps < MAX_UNSTICK_STEPS) {
                state.position.y += UNSTICK_STEP;
                tickStartPos.y += UNSTICK_STEP;
                steps++;
            }

            if (steps >= MAX_UNSTICK_STEPS) {
                // Kurtarilamadi -- egilmeyi iptal et, ayakta kalmaya devam et.
                // Haritadan dusmektense bu daha guvenli bir sonuc.
                state.position.y += HULL_DIFF;
                tickStartPos.y += HULL_DIFF;
                state.isCrouching = false;
            }
        }
        else if (!input.ctrlHeld && state.isCrouching) {
            // Kalksaydik nereye giderdik, o noktanin solid olup olmadigi
            // dogrudan kontrol edilir (trace degil -- trace'in baslangici
            // hala egik-uzayda olurdu ve hull1 ile tutarsiz sonuc verirdi).
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
    // uygulanir: yukselirken normal, duserken daha guclu yercekimi -- ziplama
    // yayini daha keskin ve "agirlikli" hissettirir.
    float gravityThisFrame = gravity;
    if (state.verticalVelocity < 0.0f) {
        gravityThisFrame *= FALL_GRAVITY_MULTIPLIER;
    }
    state.verticalVelocity -= gravityThisFrame * deltaTime;
    float verticalDelta = state.verticalVelocity * deltaTime;

    // Yatay ve dikey hareket tek bir hedef noktada birlestirilip tek bir
    // SlideMove cagrisiyla test edilir. Boylece duvara capraz yaklasirken
    // (yatay+dikey ayni anda) dogru kayma davranisi olusur.
    glm::vec3 newPos = tickStartPos + horizontalDelta + glm::vec3(0.0f, verticalDelta, 0.0f);

    if (newPos != tickStartPos) {
        glm::vec3 resolvedPos = map.SlideMove(tickStartPos, newPos, moveHull);
        state.position = resolvedPos;

        bool verticalCollisionHappened = std::abs(resolvedPos.y - newPos.y) > 0.01f;
        if (verticalCollisionHappened) {
            // Zemine gercekten carpildigi an burasidir. Havadaysa (jump ya da
            // hard-slam'den geliyorsa) olay burada bildirilir -- hiz buyuklugune
            // degil, nasil havalandigina gore, cunku bu daha ongorulebilir ve
            // ayarlanabilir bir sonuc verir.
            if (state.airborneFromHardSlam) {
                events.hardSlamLanded = true;
            }
            else if (state.airborneFromJump) {
                events.jumpLanded = true;
            }

            state.airborneFromJump = false;
            state.airborneFromHardSlam = false;
            state.verticalVelocity = 0.0f;
        }
    }
    else {
        state.position = tickStartPos;
    }

    return events;
}