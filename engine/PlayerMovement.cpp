#include "PlayerMovement.h"
#include "Camera.h"
#include "BSPMap.h"
#include "console/Console.h"
#include "console/CVar.h"
#include "../game/src/Player.h"
#include <cmath>

// Iniş sarsıntısı için sabit büyüklükler. Hız hesabına bağlı olmadığı için
// yerçekimi/zıplama kuvveti ne olursa olsun her zaman aynı, öngörülebilir
// bir his verir.
static constexpr float NORMAL_LANDING_SHAKE = 6.0f;
static constexpr float HARD_SLAM_LANDING_SHAKE = 22.0f;

void UpdatePlayerPhysics(float deltaTime, glm::vec3 oldPos) {
    g_Player.UpdateWeapons(deltaTime);

    if (g_CVar.cm_noclip) {
        g_Camera.verticalVelocity = 0.0f;
        g_Camera.isCrouching = false;
        g_Camera.UpdateViewBob(deltaTime, 0.0f, false);
        return;
    }

    if (Console::IsOpen()) {
        return;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    bool ctrlHeld = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];

    static bool spaceHeldPrevFrame = false;
    static bool ctrlHeldPrevFrame = false;
    bool spaceHeldNow = keys[SDL_SCANCODE_SPACE];
    bool spaceJustPressed = spaceHeldNow && !spaceHeldPrevFrame;
    bool ctrlJustPressed = ctrlHeld && !ctrlHeldPrevFrame;

    constexpr float HALF_STAND = 36.0f;
    constexpr float HALF_CROUCH = 18.0f;
    constexpr float HULL_DIFF = HALF_STAND - HALF_CROUCH;

    int currentHull = g_Camera.isCrouching ? 3 : 1;
    glm::vec3 groundCheckEnd = oldPos - glm::vec3(0.0f, 2.0f, 0.0f);
    TraceResult groundTrace = g_Map.TraceLine(oldPos, groundCheckEnd, currentHull);
    g_Camera.onGround = (groundTrace.fraction < 1.0f);

    if (g_Camera.onGround && g_Camera.verticalVelocity <= 0.0f) {
        g_Camera.verticalVelocity = 0.0f;
    }

    // Havada olup olmadığımızı, nasıl havaya çıktığımızı (zıplayarak mı, sert
    // inişle mi) ayrı bayraklarla takip ediyoruz. Böylece iniş sarsıntısının
    // türü, o anki dikey hıza değil, oyuncunun eylemine bağlı oluyor -- daha
    // öngörülebilir ve ayarlanabilir bir his veriyor.
    static bool airborneFromJump = false;
    static bool airborneFromHardSlam = false;

    if (spaceJustPressed && g_Camera.onGround) {
        g_Camera.verticalVelocity = g_CVar.nvs_jumpforce;
        g_Camera.onGround = false;
        airborneFromJump = true;
        airborneFromHardSlam = false;
    }

    if (ctrlJustPressed && !g_Camera.onGround && g_Player.RGDitem) {
        g_Camera.verticalVelocity = -g_CVar.nvs_jumpforce * 4.0f;
        airborneFromHardSlam = true;
        airborneFromJump = false;
    }
    else if (g_Camera.onGround) {
        if (ctrlHeld && !g_Camera.isCrouching) {
            // Origin'i asagi kaydiriyoruz ki ayaklar ayni zemin yukseklikginde kalsin.
            // oldPos'u da ayni miktarda kaydirmazsak, bu frame'in SlideMove'u
            // oldPos'u yanlis hull-uzayinda yorumlar ve haritanin altina iter.
            g_Camera.position.y -= HULL_DIFF;
            oldPos.y -= HULL_DIFF;
            g_Camera.isCrouching = true;

            // Duz zeminde bu kaydirma guvenlidir, ama egimli bir yuzeyde hull3'un
            // gercek sinir noktasi sadece dikey degil, egim normali dogrultusunda
            // kayar. Bu da yeni origin'in yuzeyin icine gomulmesine yol acabilir --
            // gomulu origin sonraki frame'lerde collision tarafindan yakalanamayip
            // oyuncunun haritadan dusmesine sebep olur. Kucuk adimlarla yukari
            // iterek gomulmeyi duzeltiyoruz.
            const float UNSTICK_STEP = 1.0f;
            const int MAX_UNSTICK_STEPS = static_cast<int>(HULL_DIFF / UNSTICK_STEP) + 2;
            int steps = 0;
            while (g_Map.IsPointSolid(g_Camera.position, 3) && steps < MAX_UNSTICK_STEPS) {
                g_Camera.position.y += UNSTICK_STEP;
                oldPos.y += UNSTICK_STEP;
                steps++;
            }

            if (steps >= MAX_UNSTICK_STEPS) {
                // Kurtarilamadi -- egilmeyi iptal et, ayakta kalmaya devam et.
                // Haritadan dusmektense bu daha guvenli bir sonuc.
                g_Camera.position.y += HULL_DIFF;
                oldPos.y += HULL_DIFF;
                g_Camera.isCrouching = false;
            }
        }
        else if (!ctrlHeld && g_Camera.isCrouching) {
            // Kalksaydik nereye giderdik, o noktanin solid olup olmadigini
            // dogrudan kontrol ediyoruz (trace degil -- trace'in baslangici
            // hala egik-uzayda olurdu ve hull1 ile tutarsiz sonuc verirdi).
            glm::vec3 candidateStandPos = g_Camera.position + glm::vec3(0.0f, HULL_DIFF, 0.0f);
            if (!g_Map.IsPointSolid(candidateStandPos, 1)) {
                g_Camera.position = candidateStandPos;
                oldPos.y += HULL_DIFF;
                g_Camera.isCrouching = false;
            }
        }
    }

    int moveHull = g_Camera.isCrouching ? 3 : 1;

    // Yercekimi entegrasyonu. Dususte ekstra bir carpan uygulaniyor: yukselirken
    // normal, duserken daha guclu yercekimi -- ziplama yayini daha keskin ve
    // "agirlikli" hissettirir, klasik arcade FPS'lerdeki gibi.
    const float FALL_GRAVITY_MULTIPLIER = 1.6f;
    float gravityThisFrame = g_CVar.nvs_gravity;
    if (g_Camera.verticalVelocity < 0.0f) {
        gravityThisFrame *= FALL_GRAVITY_MULTIPLIER;
    }
    g_Camera.verticalVelocity -= gravityThisFrame * deltaTime;
    g_Camera.position.y += g_Camera.verticalVelocity * deltaTime;

    glm::vec3 newPos = g_Camera.position;
    if (newPos != oldPos) {
        glm::vec3 resolvedPos = g_Map.SlideMove(oldPos, newPos, moveHull);
        g_Camera.position = resolvedPos;

        bool verticalCollisionHappened = std::abs(resolvedPos.y - newPos.y) > 0.01f;
        if (verticalCollisionHappened) {
            // Zemine gercekten carptigimiz an burasi. Havadaysak (jump ya da
            // hard-slam'den geliyorsak) sarsintiyi tam bu noktada tetikliyoruz --
            // hiz buyuklugune degil, nasil havalandigimiza gore sabit bir
            // buyukluk kullaniyoruz, boylece davranis her zaman ongorulebilir.
            if (airborneFromHardSlam) {
                g_Camera.TriggerLandingShake(HARD_SLAM_LANDING_SHAKE);
                Console::Log("Landing shake: HARD_SLAM");
            }
            else if (airborneFromJump) {
                g_Camera.TriggerLandingShake(NORMAL_LANDING_SHAKE);
                Console::Log("Landing shake: NORMAL");
            }

            airborneFromJump = false;
            airborneFromHardSlam = false;
            g_Camera.verticalVelocity = 0.0f;
        }
    }

    float horizDist = glm::length(glm::vec2(g_Camera.position.x - oldPos.x, g_Camera.position.z - oldPos.z));
    float horizSpeed = (deltaTime > 0.0001f) ? (horizDist / deltaTime) : 0.0f;
    g_Camera.UpdateViewBob(deltaTime, horizSpeed, g_Camera.onGround);

    spaceHeldPrevFrame = spaceHeldNow;
    ctrlHeldPrevFrame = ctrlHeld;
}