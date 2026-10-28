#include "pch.h"
#include "Combat.h"
#include "ServerContext.h"
#include "BotManager.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace {
    // ---------------------------------------------------------------
    // Dengeleme tablosu. Birimler: dunya birimi, saniye, hasar puani.
    // ---------------------------------------------------------------

    // --- Genel ---
    constexpr float MAX_RANGE = 8192.0f;           // bir anlik atisin ulasabilecegi en uzak mesafe
    constexpr float GRAVITY = 800.0f;              // bombalarin asagi ivmesi (birim/sn^2)
    constexpr float BOUNCE_RESTITUTION = 0.45f;    // sekmede duvara dik hizin ne kadari korunur (0 = yapisir, 1 = kusursuz seker)
    constexpr float BOUNCE_TANGENT_KEEP = 0.75f;   // sekmede duvar boyunca kayan hizin ne kadari korunur (surtunme)

    // --- Roket ---
    constexpr float ROCKET_SPEED = 1000.0f;        // ucus hizi (birim/sn)
    constexpr float ROCKET_RADIUS = 180.0f;        // patlama yaricapi: bu mesafede hasar sifira iner
    constexpr int ROCKET_DAMAGE = 130;             // patlama merkezindeki azami hasar (mesafeyle dogrusal azalir)
    constexpr float ROCKET_LIFE = 8.0f;            // hicbir sey vurmazsa kac saniye sonra yok olur

    // --- Bombalar (el bombasi, sis, lav, asit) ---
    constexpr float BOMB_SPEED = 720.0f;           // atis hizi (birim/sn)
    constexpr float BOMB_LIFE = 6.0f;              // en uzun omur (sn)
    constexpr float GRENADE_FUSE = 2.5f;           // el bombasinin patlama gecikmesi (sn)
    constexpr float GRENADE_RADIUS = 220.0f;       // el bombasi patlama yaricapi
    constexpr int GRENADE_DAMAGE = 150;            // el bombasi azami hasari
    constexpr float FOG_FUSE = 1.6f;               // sis bombasinin bulut acma gecikmesi (sn); durursa daha erken acilir
    constexpr float FOG_RADIUS = 240.0f;           // sis bulutunun yaricapi
    constexpr float FOG_LIFE = 12.0f;              // sis bulutunun omru (sn)
    constexpr float LAVA_IMPACT_RADIUS = 130.0f;   // lav bombasinin carpma patlamasi yaricapi
    constexpr int LAVA_IMPACT_DAMAGE = 35;         // carpma patlamasi hasari
    constexpr float LAVA_POOL_RADIUS = 150.0f;     // lav havuzunun yaricapi
    constexpr float LAVA_POOL_LIFE = 7.0f;         // havuzun omru (sn)
    constexpr float LAVA_TICK_INTERVAL = 0.5f;     // havuz icindekilere kac saniyede bir hasar verilir
    constexpr int LAVA_TICK_DAMAGE = 8;            // her seferde verilen hasar
    constexpr float ACID_IMPACT_RADIUS = 120.0f;   // asit bombasi carpma patlamasi yaricapi
    constexpr int ACID_IMPACT_DAMAGE = 25;
    constexpr float ACID_POOL_RADIUS = 140.0f;
    constexpr float ACID_POOL_LIFE = 8.0f;
    constexpr float ACID_TICK_INTERVAL = 0.4f;     // asit lavdan sik ama hafif vurur
    constexpr int ACID_TICK_DAMAGE = 6;

    // --- Testere ---
    constexpr float SAW_RADIUS = 20.0f;            // hedefe temas yaricapi (hitbox'a eklenir)
    constexpr float SAW_LIFE = 4.0f;               // en uzun omur (sn)
    constexpr float SAW_SPEED_MIN = 1100.0f;       // hic donmemis testerenin hizi (birim/sn)
    constexpr float SAW_SPEED_MAX = 1700.0f;       // tam donmus testerenin hizi (donus arttikca hizlanir)

    // --- Staplegun ---
    constexpr float STAPLE_SPEED = 1200.0f;        // top hizi (birim/sn)
    constexpr float STAPLE_RADIUS = 6.0f;          // temas yaricapi
    constexpr int STAPLE_DAMAGE = 18;              // topun tek vurus hasari (bir salvo 5 top)
    constexpr float STAPLE_LIFE = 3.0f;            // en uzun omur (sn)

    // --- Lastgun ---
    constexpr float LAST_SPEED = 520.0f;           // top hizi (birim/sn): yavas ama devasa
    constexpr float LAST_RADIUS = 40.0f;           // temas yaricapi
    constexpr float LAST_EXPLOSION_RADIUS = 480.0f;// patlama yaricapi
    constexpr int LAST_DAMAGE = 450;               // patlama merkezi hasari
    constexpr float LAST_LIFE = 8.0f;              // en uzun omur (sn)

    // --- Raygun gorsel isin namlusu (gozun gore) ---
    constexpr float RAY_MUZZLE_FORWARD = 30.0f;    // namlu gozun ne kadar onunde
    constexpr float RAY_MUZZLE_RIGHT = 14.0f;      // namlu gozun ne kadar sagindadir
    constexpr float RAY_MUZZLE_DOWN = 12.0f;       // namlu gozun ne kadar altindadir (isin capraz gorunur)

    // --- Sinirlar ---
    constexpr size_t MAX_PROJECTILES = 128;        // ayni anda en fazla mermi
    constexpr size_t MAX_EFFECTS = 64;             // ayni anda en fazla gorsel etki (patlama, isin)
    constexpr size_t MAX_HAZARDS = 16;             // ayni anda en fazla sis bulutu / havuz
    constexpr int FIRST_ENTITY_ID = 1000000;       // bot kimlikleriyle (1'den baslar) cakismasin

    // windows.h'in min/max makrolariyla cakismamak icin yerel yardimcilar.
    float MinF(float a, float b) { return a < b ? a : b; }
    float MaxF(float a, float b) { return a > b ? a : b; }
    float ClampF(float v, float lo, float hi) { return MaxF(lo, MinF(hi, v)); }

    std::mt19937& Rng() {
        static std::mt19937 rng{ std::random_device{}() };
        return rng;
    }

    float RandomRange(float lo, float hi) {
        return std::uniform_real_distribution<float>(lo, hi)(Rng());
    }

    float YawOf(const glm::vec3& d) { return glm::degrees(std::atan2(d.z, d.x)); }
    float PitchOf(const glm::vec3& d) { return glm::degrees(std::asin(ClampF(d.y, -1.0f, 1.0f))); }

    // ---------------------------------------------------------------
    // Veri yapilari
    // ---------------------------------------------------------------
    struct Projectile {
        int id = 0;
        uint16_t classId = ENT_CLASS_NONE;
        int owner = -1;
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };
        glm::vec3 dir{ 1.0f, 0.0f, 0.0f };
        float age = 0.0f;
        float life = 5.0f;
        float fuse = 0.0f;     // 0: fitil yok
        float radius = 0.0f;   // hedef carpisma yaricapi
        int damage = 0;
        float spin = 0.0f;     // testere donusu 0..1
        bool gravity = false;
        bool pierce = false;
        bool resting = false;
        bool dead = false;
        std::vector<uintptr_t> hitKeys; // delen mermiler ayni hedefe bir kez hasar verir
    };

    struct Effect {
        int id = 0;
        uint16_t classId = ENT_CLASS_NONE;
        glm::vec3 pos{ 0.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;
        float age = 0.0f;
        float life = 0.5f;
        uint16_t size = 0;
    };

    struct Hazard {
        int id = 0;
        uint16_t classId = ENT_CLASS_NONE;
        int owner = -1;
        glm::vec3 pos{ 0.0f };
        float radius = 0.0f;
        float age = 0.0f;
        float life = 5.0f;
        float tick = 0.0f;
    };

    // Hasar alabilen bir hedef: bot ya da oyuncu. Kutu, merkez +- yari boyuttur.
    struct Target {
        e_BotBase* bot = nullptr;
        int slot = -1;
        glm::vec3 center{ 0.0f };
        glm::vec3 half{ 0.0f };

        uintptr_t Key() const {
            return bot != nullptr ? reinterpret_cast<uintptr_t>(bot) : static_cast<uintptr_t>(slot + 1);
        }
    };

    std::vector<Projectile> s_projectiles;
    std::vector<Effect> s_effects;
    std::vector<Hazard> s_hazards;
    int s_nextId = FIRST_ENTITY_ID;

    int NextId() { return s_nextId++; }

    // ---------------------------------------------------------------
    // Hedefler ve geometri
    // ---------------------------------------------------------------
    std::vector<Target> GatherTargets() {
        std::vector<Target> targets;

        for (const auto& bot : BotManager::GetBots()) {
            if (!bot->IsAlive()) continue;

            Target target;
            target.bot = bot.get();
            target.center = bot->GetPosition();
            target.half = bot->GetHalfExtents();
            targets.push_back(target);
        }

        // Oyuncu konumu server'da yalnizca goz konumu olarak bilinir; hull (+-16, +-36, +-16)
        // ayaktayken gozun 28 birim asagisini merkez alir (cok oyunculuda hareket
        // server'a tasininca kesinlesecek bir yaklasim).
        ServerContext& ctx = Ctx();
        for (int i = 0; i < ServerContext::MAX_PLAYERS; i++) {
            PlayerSlot& slot = ctx.slots[i];
            if (!slot.connected || slot.player.Health <= 0) continue;

            Target target;
            target.slot = i;
            target.center = slot.eyePosition - glm::vec3(0.0f, 28.0f, 0.0f);
            target.half = glm::vec3(16.0f, 36.0f, 16.0f);
            targets.push_back(target);
        }
        return targets;
    }

    void DamageTarget(const Target& target, int amount) {
        if (amount <= 0) return;

        if (target.bot != nullptr) target.bot->TakeDamage(amount);
        else if (target.slot >= 0) Ctx().slots[target.slot].player.TakeDamage(amount);
    }

    // Isin ile eksen hizali kutu kesisimi (slab yontemi). tOut: isin boyunca ilk temas mesafesi.
    bool RayAABB(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& mn, const glm::vec3& mx,
        float maxT, float& tOut) {
        float tMin = 0.0f;
        float tMax = maxT;

        for (int axis = 0; axis < 3; axis++) {
            if (std::fabs(dir[axis]) < 1e-8f) {
                if (origin[axis] < mn[axis] || origin[axis] > mx[axis]) return false;
            }
            else {
                float t1 = (mn[axis] - origin[axis]) / dir[axis];
                float t2 = (mx[axis] - origin[axis]) / dir[axis];
                if (t1 > t2) { const float tmp = t1; t1 = t2; t2 = tmp; }

                tMin = MaxF(tMin, t1);
                tMax = MinF(tMax, t2);
                if (tMin > tMax) return false;
            }
        }
        tOut = tMin;
        return true;
    }

    float DistancePointAABB(const glm::vec3& p, const glm::vec3& mn, const glm::vec3& mx) {
        const glm::vec3 closest(
            ClampF(p.x, mn.x, mx.x), ClampF(p.y, mn.y, mx.y), ClampF(p.z, mn.z, mx.z));
        return glm::length(p - closest);
    }

    // Noktanin altindaki zemini bulur (lav havuzu icin); bulunamazsa noktanin kendisi.
    glm::vec3 GroundPoint(const glm::vec3& point) {
        IWorldQuery* world = Ctx().world;
        if (world == nullptr) return point;

        const TraceResult trace = world->TraceLine(point, point - glm::vec3(0.0f, 600.0f, 0.0f), 0);
        if (trace.fraction >= 1.0f) return point;
        return trace.endPos + glm::vec3(0.0f, 2.0f, 0.0f);
    }

    // ---------------------------------------------------------------
    // Etkiler, tehlike alanlari, patlama
    // ---------------------------------------------------------------
    void AddEffect(uint16_t classId, const glm::vec3& pos, float yaw, float pitch, float life, uint16_t size) {
        if (s_effects.size() >= MAX_EFFECTS) s_effects.erase(s_effects.begin());

        Effect effect;
        effect.id = NextId();
        effect.classId = classId;
        effect.pos = pos;
        effect.yaw = yaw;
        effect.pitch = pitch;
        effect.life = life;
        effect.size = size;
        s_effects.push_back(effect);
    }

    void SpawnHazard(uint16_t classId, int owner, const glm::vec3& pos, float radius, float life) {
        if (s_hazards.size() >= MAX_HAZARDS) s_hazards.erase(s_hazards.begin());

        Hazard hazard;
        hazard.id = NextId();
        hazard.classId = classId;
        hazard.owner = owner;
        hazard.pos = pos;
        hazard.radius = radius;
        hazard.life = life;
        s_hazards.push_back(hazard);
    }

    // Alan hasari: hedef kutusuna en yakin mesafeye gore dogrusal azalir. Patlama ile hedef
    // arasinda duvar varsa hasar verilmez; sahibi kendi patlamasindan yariya yakin hasar alir.
    void Explode(int owner, const glm::vec3& pos, float radius, int maxDamage,
        uint16_t effectClass = ENT_CLASS_EXPLOSION, float effectLife = EXPLOSION_EFFECT_LIFE) {
        if (effectClass != ENT_CLASS_NONE) {
            AddEffect(effectClass, pos, 0.0f, 0.0f, effectLife, static_cast<uint16_t>(radius));
        }
        IWorldQuery* world = Ctx().world;
        for (const Target& target : GatherTargets()) {
            const float distance = DistancePointAABB(pos, target.center - target.half, target.center + target.half);
            if (distance >= radius) continue;

            if (world != nullptr && distance > 1.0f) {
                const TraceResult trace = world->TraceLine(pos, target.center, 0);
                if (trace.fraction < 0.98f) continue;
            }

            int damage = static_cast<int>(static_cast<float>(maxDamage) * (1.0f - distance / radius));
            if (target.slot == owner) damage /= 2;
            DamageTarget(target, damage);
        }
    }

    // Lav ve asit bombasi: kucuk alan hasari + zeminde havuz.
    void LavaImpact(Projectile& projectile, const glm::vec3& point) {
        const bool acid = (projectile.classId == ENT_CLASS_ACID_BOMB);

        Explode(projectile.owner, point,
            acid ? ACID_IMPACT_RADIUS : LAVA_IMPACT_RADIUS,
            acid ? ACID_IMPACT_DAMAGE : LAVA_IMPACT_DAMAGE,
            ENT_CLASS_NONE);

        SpawnHazard(acid ? ENT_CLASS_ACID_POOL : ENT_CLASS_LAVA_POOL, projectile.owner, GroundPoint(point),
            acid ? ACID_POOL_RADIUS : LAVA_POOL_RADIUS,
            acid ? ACID_POOL_LIFE : LAVA_POOL_LIFE);
        projectile.dead = true;
    }

    // ---------------------------------------------------------------
    // Mermiler
    // ---------------------------------------------------------------
    Projectile* AddProjectile(uint16_t classId, int owner, const glm::vec3& pos, const glm::vec3& dir, float speed) {
        if (s_projectiles.size() >= MAX_PROJECTILES) return nullptr;

        Projectile projectile;
        projectile.id = NextId();
        projectile.classId = classId;
        projectile.owner = owner;
        projectile.pos = pos;
        projectile.dir = dir;
        projectile.vel = dir * speed;
        s_projectiles.push_back(projectile);
        return &s_projectiles.back();
    }

    void Bounce(Projectile& p, const TraceResult& trace) {
        glm::vec3 normal = trace.planeNormal;
        if (glm::length(normal) < 0.5f) normal = glm::vec3(0.0f, 1.0f, 0.0f);

        p.pos = trace.endPos + normal;

        const float vn = glm::dot(p.vel, normal);
        const glm::vec3 reflected = p.vel - (1.0f + BOUNCE_RESTITUTION) * vn * normal;
        const glm::vec3 normalPart = glm::dot(reflected, normal) * normal;
        p.vel = normalPart + (reflected - normalPart) * BOUNCE_TANGENT_KEEP;

        // Zeminde yavaslayan bomba durur; sis bombasi bu anda patlar.
        if (normal.y > 0.7f && glm::length(p.vel) < 80.0f) {
            p.vel = glm::vec3(0.0f);
            p.resting = true;
        }
    }

    // Fitili ya da omru dolan bomba: el bombasi patlar, sis bombasi bulut acar.
    void Detonate(Projectile& p) {
        if (p.classId == ENT_CLASS_GRENADE) {
            Explode(p.owner, p.pos, GRENADE_RADIUS, GRENADE_DAMAGE);
        }
        else if (p.classId == ENT_CLASS_FOG_BOMB) {
            SpawnHazard(ENT_CLASS_FOG_CLOUD, p.owner, p.pos, FOG_RADIUS, FOG_LIFE);
        }
        p.dead = true;
    }

    // Bir merminin hedefe temasi. true donerse mermi bitmistir.
    bool OnTargetHit(Projectile& p, const Target& target, const glm::vec3& hitPoint) {
        switch (p.classId) {
        case ENT_CLASS_ROCKET:
            Explode(p.owner, hitPoint, ROCKET_RADIUS, ROCKET_DAMAGE);
            p.dead = true;
            break;
        case ENT_CLASS_LAST_BALL:
            Explode(p.owner, hitPoint, LAST_EXPLOSION_RADIUS, LAST_DAMAGE, ENT_CLASS_BFG_EXPLOSION, BFG_EXPLOSION_LIFE);
            p.dead = true;
            break;
        case ENT_CLASS_STAPLE_BALL:
            DamageTarget(target, p.damage);
            p.dead = true;
            break;
        case ENT_CLASS_SAWBLADE:
            DamageTarget(target, p.damage);
            p.hitKeys.push_back(target.Key());
            break;
        case ENT_CLASS_GRENADE:
            // Hedefe dogrudan carpan el bombasi aninda patlar (atis ani temas sayilmaz).
            if (p.age > 0.08f) {
                Explode(p.owner, hitPoint, GRENADE_RADIUS, GRENADE_DAMAGE);
                p.dead = true;
            }
            break;
        case ENT_CLASS_ACID_BOMB:
        case ENT_CLASS_LAVA_BOMB:
            LavaImpact(p, hitPoint);
            break;
        default:
            break; // sis bombasi hedeflerin icinden gecer
        }
        return p.dead;
    }

    void OnWorldHit(Projectile& p, const TraceResult& trace) {
        glm::vec3 normal = trace.planeNormal;
        if (glm::length(normal) < 0.5f) normal = glm::vec3(0.0f, 1.0f, 0.0f);

        switch (p.classId) {
        case ENT_CLASS_ROCKET:
            Explode(p.owner, trace.endPos + normal * 6.0f, ROCKET_RADIUS, ROCKET_DAMAGE);
            p.dead = true;
            break;
        case ENT_CLASS_LAST_BALL:
            Explode(p.owner, trace.endPos + normal * 20.0f, LAST_EXPLOSION_RADIUS, LAST_DAMAGE, ENT_CLASS_BFG_EXPLOSION, BFG_EXPLOSION_LIFE);
            p.dead = true;
            break;
        case ENT_CLASS_STAPLE_BALL:
        case ENT_CLASS_SAWBLADE:
            p.dead = true;
            break;
        case ENT_CLASS_ACID_BOMB:
        case ENT_CLASS_LAVA_BOMB:
            LavaImpact(p, trace.endPos + normal * 2.0f);
            break;
        case ENT_CLASS_GRENADE:
        case ENT_CLASS_FOG_BOMB:
            Bounce(p, trace);
            break;
        default:
            p.dead = true;
            break;
        }
    }

    void StepProjectile(Projectile& p, float dt, const std::vector<Target>& targets) {
        p.age += dt;

        if (p.classId == ENT_CLASS_GRENADE && p.age >= p.fuse) { Detonate(p); return; }
        if (p.classId == ENT_CLASS_FOG_BOMB && (p.age >= p.fuse || p.resting)) { Detonate(p); return; }
        if (p.age >= p.life) { p.dead = true; return; }
        if (p.resting) return;

        if (p.gravity) p.vel.y -= GRAVITY * dt;

        const glm::vec3 oldPos = p.pos;
        const glm::vec3 newPos = oldPos + p.vel * dt;
        const glm::vec3 delta = newPos - oldPos;
        const float segLen = glm::length(delta);
        if (segLen < 0.0001f) return;

        const glm::vec3 dir = delta / segLen;
        p.dir = dir;

        // Dunya: nokta hull'u ile iz surulur.
        TraceResult trace;
        bool hitWorld = false;
        float worldDist = segLen + 1.0f;
        if (IWorldQuery* world = Ctx().world) {
            trace = world->TraceLine(oldPos, newPos, 0);
            if (trace.fraction < 1.0f) {
                hitWorld = true;
                worldDist = trace.fraction * segLen;
            }
        }

        // Hedefler: dunya carpmasindan once temas edenler, temas sirasina gore.
        std::vector<std::pair<float, const Target*>> hits;
        const float reach = hitWorld ? worldDist : segLen;
        const glm::vec3 pad(p.radius);
        for (const Target& target : targets) {
            if (target.slot == p.owner) continue; // sahibine dogrudan carpmaz
            if (p.pierce && std::find(p.hitKeys.begin(), p.hitKeys.end(), target.Key()) != p.hitKeys.end()) continue;

            float tHit = 0.0f;
            if (RayAABB(oldPos, dir, target.center - target.half - pad, target.center + target.half + pad, reach, tHit)) {
                hits.emplace_back(tHit, &target);
            }
        }
        std::sort(hits.begin(), hits.end(),
            [](const std::pair<float, const Target*>& a, const std::pair<float, const Target*>& b) { return a.first < b.first; });

        for (const auto& hit : hits) {
            if (OnTargetHit(p, *hit.second, oldPos + dir * hit.first)) return;
        }

        if (hitWorld) {
            OnWorldHit(p, trace);
            return;
        }

        p.pos = newPos;
    }
}

// -------------------------------------------------------------------
// Genel arayuz
// -------------------------------------------------------------------
void Combat::Reset() {
    s_projectiles.clear();
    s_effects.clear();
    s_hazards.clear();
    s_nextId = FIRST_ENTITY_ID;
}

void Combat::Update(float deltaTime) {
    if (!s_projectiles.empty()) {
        const std::vector<Target> targets = GatherTargets();
        for (Projectile& projectile : s_projectiles) {
            if (!projectile.dead) StepProjectile(projectile, deltaTime, targets);
        }
        s_projectiles.erase(
            std::remove_if(s_projectiles.begin(), s_projectiles.end(),
                [](const Projectile& p) { return p.dead; }),
            s_projectiles.end());
    }

    for (Effect& effect : s_effects) effect.age += deltaTime;
    s_effects.erase(
        std::remove_if(s_effects.begin(), s_effects.end(),
            [](const Effect& e) { return e.age >= e.life; }),
        s_effects.end());

    std::vector<Target> hazardTargets;
    bool gathered = false;
    for (Hazard& hazard : s_hazards) {
        hazard.age += deltaTime;

        const bool isLava = (hazard.classId == ENT_CLASS_LAVA_POOL);
        const bool isAcid = (hazard.classId == ENT_CLASS_ACID_POOL);
        if (!isLava && !isAcid) continue;

        const float tickInterval = isLava ? LAVA_TICK_INTERVAL : ACID_TICK_INTERVAL;
        hazard.tick += deltaTime;
        if (hazard.tick < tickInterval) continue;
        hazard.tick -= tickInterval;

        if (!gathered) {
            hazardTargets = GatherTargets();
            gathered = true;
        }
        for (const Target& target : hazardTargets) {
            const float dx = target.center.x - hazard.pos.x;
            const float dz = target.center.z - hazard.pos.z;
            if (dx * dx + dz * dz > hazard.radius * hazard.radius) continue;

            // Havuz zeminden 48 birim yukseklige kadar etkilidir.
            const float feet = target.center.y - target.half.y;
            if (feet > hazard.pos.y + 48.0f || target.center.y + target.half.y < hazard.pos.y) continue;

            DamageTarget(target, LAVA_TICK_DAMAGE);
        }
    }
    s_hazards.erase(
        std::remove_if(s_hazards.begin(), s_hazards.end(),
            [](const Hazard& h) { return h.age >= h.life; }),
        s_hazards.end());
}

int Combat::CollectEntityStates(NetEntityState* outStates, int maxCount) {
    int count = 0;

    for (const Projectile& p : s_projectiles) {
        if (count >= maxCount) return count;

        NetEntityState& state = outStates[count++];
        state = NetEntityState();
        state.id = p.id;
        state.classId = p.classId;
        state.origin = p.pos;
        state.yaw = YawOf(p.dir);
        state.pitch = PitchOf(p.dir);
        if (p.classId == ENT_CLASS_SAWBLADE) state.flags = static_cast<uint16_t>(p.spin * 100.0f);
    }

    for (const Effect& e : s_effects) {
        if (count >= maxCount) return count;

        NetEntityState& state = outStates[count++];
        state = NetEntityState();
        state.id = e.id;
        state.classId = e.classId;
        state.origin = e.pos;
        state.yaw = e.yaw;
        state.pitch = e.pitch;
        state.flags = e.size;
        state.health = static_cast<int16_t>(e.age * 100.0f);
    }

    for (const Hazard& h : s_hazards) {
        if (count >= maxCount) return count;

        NetEntityState& state = outStates[count++];
        state = NetEntityState();
        state.id = h.id;
        state.classId = h.classId;
        state.origin = h.pos;
        state.pitch = h.life;
        state.flags = static_cast<uint16_t>(h.radius);
        state.health = static_cast<int16_t>(h.age * 100.0f);
    }

    return count;
}

glm::vec3 Combat::MuzzlePoint(const glm::vec3& eye, const glm::vec3& direction, float distance) {
    const glm::vec3 target = eye + direction * distance;

    if (IWorldQuery* world = Ctx().world) {
        const TraceResult trace = world->TraceLine(eye, target, 0);
        if (trace.fraction < 1.0f) {
            return eye + direction * MaxF(0.0f, trace.fraction * distance - 2.0f);
        }
    }
    return target;
}

void Combat::FireBullets(int owner, const glm::vec3& eye, const glm::vec3& forward,
    const glm::vec3& right, const glm::vec3& up,
    int pellets, float spreadDegrees, int damagePerPellet, float headshotMultiplier) {

    const std::vector<Target> targets = GatherTargets();
    IWorldQuery* world = Ctx().world;
    const float spreadTan = std::tan(glm::radians(spreadDegrees));

    for (int i = 0; i < pellets; i++) {
        // Konik dagilim: birim disk icinde rastgele nokta.
        float sx = 0.0f, sy = 0.0f;
        if (spreadTan > 0.0f) {
            do {
                sx = RandomRange(-1.0f, 1.0f);
                sy = RandomRange(-1.0f, 1.0f);
            } while (sx * sx + sy * sy > 1.0f);
        }
        const glm::vec3 dir = glm::normalize(forward + right * (sx * spreadTan) + up * (sy * spreadTan));

        float wallDistance = MAX_RANGE;
        if (world != nullptr) {
            const TraceResult trace = world->TraceLine(eye, eye + dir * MAX_RANGE, 0);
            wallDistance = trace.fraction * MAX_RANGE;
        }

        const Target* best = nullptr;
        float bestT = wallDistance;
        for (const Target& target : targets) {
            if (target.slot == owner) continue;

            float tHit = 0.0f;
            if (RayAABB(eye, dir, target.center - target.half, target.center + target.half, bestT, tHit) && tHit < bestT) {
                best = &target;
                bestT = tHit;
            }
        }

        if (best != nullptr) {
            const glm::vec3 hitPoint = eye + dir * bestT;
            const bool headshot = hitPoint.y >= best->center.y + best->half.y * 0.5f;
            DamageTarget(*best, static_cast<int>(static_cast<float>(damagePerPellet) * (headshot ? headshotMultiplier : 1.0f)));
        }
    }
}

void Combat::FireRay(int owner, const glm::vec3& eye, const glm::vec3& forward, int damage) {
    IWorldQuery* world = Ctx().world;

    // Isin ilk duvara ya da azami menzile kadar gider.
    float wallDistance = MAX_RANGE;
    if (world != nullptr) {
        const TraceResult trace = world->TraceLine(eye, eye + forward * MAX_RANGE, 0);
        wallDistance = trace.fraction * MAX_RANGE;
    }

    // Hasar, nisan dogrusu boyunca verilir: yoldaki herkes etkilenir.
    for (const Target& target : GatherTargets()) {
        if (target.slot == owner) continue;

        float tHit = 0.0f;
        if (RayAABB(eye, forward, target.center - target.half, target.center + target.half, wallDistance, tHit)) {
            DamageTarget(target, damage);
        }
    }

    // Gorsel isin: namlu gozun sag-alt onundedir, isin nisan noktasina ulasir. Boylece
    // oyuncu isini ONDEN degil capraz gorur; gozle ayni dogrultuda cikan isin tek cizgiye
    // dusup gorunmezdi.
    glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    right = (glm::length(right) > 0.001f) ? glm::normalize(right) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 up = glm::cross(right, forward);

    const glm::vec3 muzzle = eye + forward * RAY_MUZZLE_FORWARD + right * RAY_MUZZLE_RIGHT - up * RAY_MUZZLE_DOWN;
    const glm::vec3 impact = eye + forward * wallDistance;

    const glm::vec3 beamVector = impact - muzzle;
    const float beamLength = glm::length(beamVector);
    const glm::vec3 beamDirection = (beamLength > 1.0f) ? beamVector / beamLength : forward;

    AddEffect(ENT_CLASS_BEAM, muzzle, YawOf(beamDirection), PitchOf(beamDirection), BEAM_EFFECT_LIFE,
        static_cast<uint16_t>(ClampF(beamLength, 0.0f, 8191.0f)));   // uzunluk 16 bit alana sigmali
}

void Combat::SpawnRocket(int owner, const glm::vec3& position, const glm::vec3& direction) {
    if (Projectile* p = AddProjectile(ENT_CLASS_ROCKET, owner, position, direction, ROCKET_SPEED)) {
        p->life = ROCKET_LIFE;
    }
}

void Combat::SpawnBomb(int owner, const glm::vec3& position, const glm::vec3& direction, int bombKind) {
    uint16_t classId = ENT_CLASS_GRENADE;
    if (bombKind == BOMB_FOG) classId = ENT_CLASS_FOG_BOMB;
    else if (bombKind == BOMB_LAVA) classId = ENT_CLASS_LAVA_BOMB;
    else if (bombKind == BOMB_ACID) classId = ENT_CLASS_ACID_BOMB;

    if (Projectile* p = AddProjectile(classId, owner, position, direction, BOMB_SPEED)) {
        p->life = BOMB_LIFE;
        p->gravity = true;
        p->radius = 8.0f;
        if (classId == ENT_CLASS_GRENADE) p->fuse = GRENADE_FUSE;
        if (classId == ENT_CLASS_FOG_BOMB) p->fuse = FOG_FUSE;
    }
}

void Combat::SpawnSawblade(int owner, const glm::vec3& position, const glm::vec3& direction, float spin, int damage) {
    const float speed = SAW_SPEED_MIN + (SAW_SPEED_MAX - SAW_SPEED_MIN) * ClampF(spin, 0.0f, 1.0f);
    if (Projectile* p = AddProjectile(ENT_CLASS_SAWBLADE, owner, position, direction, speed)) {
        p->life = SAW_LIFE;
        p->radius = SAW_RADIUS;
        p->damage = damage;
        p->spin = ClampF(spin, 0.0f, 1.0f);
        p->pierce = true;
    }
}

void Combat::SpawnStapleBall(int owner, const glm::vec3& position, const glm::vec3& direction) {
    if (Projectile* p = AddProjectile(ENT_CLASS_STAPLE_BALL, owner, position, direction, STAPLE_SPEED)) {
        p->life = STAPLE_LIFE;
        p->radius = STAPLE_RADIUS;
        p->damage = STAPLE_DAMAGE;
    }
}

void Combat::SpawnLastBall(int owner, const glm::vec3& position, const glm::vec3& direction) {
    if (Projectile* p = AddProjectile(ENT_CLASS_LAST_BALL, owner, position, direction, LAST_SPEED)) {
        p->life = LAST_LIFE;
        p->radius = LAST_RADIUS;
    }
}

int Combat::PickRandomBombKind() {
    return std::uniform_int_distribution<int>(0, BOMB_KIND_COUNT - 1)(Rng());
}

bool Combat::IsPositionObscured(const glm::vec3& position) {
    for (const Hazard& hazard : s_hazards) {
        if (hazard.classId != ENT_CLASS_FOG_CLOUD) continue;

        const glm::vec3 d = position - hazard.pos;
        if (glm::dot(d, d) < hazard.radius * hazard.radius) return true;
    }
    return false;
}