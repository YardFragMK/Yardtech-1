#include "pch.h"
#include "ServerPlayer.h"
#include "Weapons.h"
#include "Combat.h"
#include <cmath>

namespace {
    float MinF(float a, float b) { return a < b ? a : b; }
    float MaxF(float a, float b) { return a > b ? a : b; }
    int MinI(int a, int b) { return a < b ? a : b; }

    // ---------------------------------------------------------------
    // Dengeleme tablosu: ates araligi (sn), hasar, dagilim (derece)
    // ---------------------------------------------------------------
    constexpr float SWITCH_DELAY = 0.35f;

    constexpr float PISTOL_INTERVAL = 0.30f;
    constexpr int PISTOL_DAMAGE = 12;
    constexpr float PISTOL_SPREAD = 0.7f;

    constexpr float DEAGLE_INTERVAL = 0.55f;
    constexpr int DEAGLE_DAMAGE = 45;
    constexpr float DEAGLE_HEADSHOT = 3.0f;
    constexpr float DEAGLE_SPREAD = 0.3f;

    constexpr int SHOTGUN_PELLETS = 8;
    constexpr int SHOTGUN_PELLET_DAMAGE = 9;
    constexpr float SHOTGUN_SPREAD = 4.5f;
    constexpr float SHOTGUN_ALT_SPREAD = 7.0f;
    constexpr float SHOTGUN_INTERVAL = 0.85f;
    constexpr float SHOTGUN_ALT_INTERVAL = 1.1f;

    constexpr int DOUBLE_PELLETS = 10;
    constexpr int DOUBLE_PELLET_DAMAGE = 10;
    constexpr float DOUBLE_SPREAD = 5.0f;
    constexpr float DOUBLE_ALT_SPREAD = 7.0f;
    constexpr float DOUBLE_INTERVAL = 0.35f;
    constexpr float DOUBLE_ALT_INTERVAL = 0.7f;

    constexpr float SAW_SPIN_UP_TIME = 3.0f;    // sag tik basili: tam donus suresi
    constexpr float SAW_SPIN_DECAY_TIME = 6.0f; // birakilinca donus sonme suresi
    constexpr float SAW_MIN_DAMAGE = 30.0f;
    constexpr float SAW_MAX_DAMAGE = 220.0f;
    constexpr float SAW_INTERVAL = 0.8f;

    constexpr float BOMB_INTERVAL = 0.9f;
    constexpr float ROCKET_INTERVAL = 0.8f;

    constexpr float STAPLE_INTERVAL = 0.45f;
    constexpr int STAPLE_COUNT = 5;
    constexpr float STAPLE_SPACING = 26.0f; // toplar arasi yatay mesafe

    constexpr float RAY_INTERVAL = 1.4f;
    constexpr int RAY_DAMAGE = 150;

    constexpr float LASTGUN_CHARGE_TIME = 2.0f;
    constexpr float LASTGUN_FOV_BOOST = 40.0f;
    constexpr float LASTGUN_INTERVAL = 1.6f;
    constexpr float FOV_RISE_RATE = 450.0f;  // derece/sn: "cok ani"
    constexpr float FOV_FALL_RATE = 700.0f;

    // Nisan acilarindan ileri/sag/yukari vektorleri (Camera ile ayni kural).
    void ComputeAim(float yawDegrees, float pitchDegrees, glm::vec3& forward, glm::vec3& right, glm::vec3& up) {
        const float yaw = glm::radians(yawDegrees);
        const float pitch = glm::radians(pitchDegrees);

        forward = glm::normalize(glm::vec3(
            std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)));
        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, forward));
    }

    float Approach(float current, float target, float maxDelta) {
        if (current < target) return MinF(current + maxDelta, target);
        return MaxF(current - maxDelta, target);
    }
}

ServerPlayer::ServerPlayer() {
    for (int i = 0; i < WEAPON_COUNT; i++) {
        const WeaponDef& def = GetWeaponDef(i);
        m_ammo[i].clip = def.startClip;
        m_ammo[i].reserve = def.startReserve;
    }
}

void ServerPlayer::TakeDamage(int enemyDamage) {
    if (enemyDamage <= 0 || Health <= 0) return;

    if (Armor > 0) {
        int armorDamage = enemyDamage * 70 / 100;
        int healthDamage = enemyDamage * 30 / 100;

        if (Armor >= armorDamage) {
            Armor -= armorDamage;
            Health -= healthDamage;
        }
        else {
            int remaining = armorDamage - Armor;
            Armor = 0;
            Health -= healthDamage + remaining;
        }
    }
    else {
        Health -= enemyDamage;
    }

    if (Health <= 0) {
        Health = 0;
        Death();
    }
}

void ServerPlayer::QueueActions(const PlayerActionCommand& actions) {
    m_input.attack1Count += actions.attack1Count;
    m_input.attack2Count += actions.attack2Count;
    m_input.reloadCount += actions.reloadCount;
    if (actions.switchToWeapon >= 0) m_input.switchToWeapon = actions.switchToWeapon;

    m_input.attack1Held = actions.attack1Held;
    m_input.attack2Held = actions.attack2Held;
    m_input.aimYaw = actions.aimYaw;
    m_input.aimPitch = actions.aimPitch;
}

void ServerPlayer::CancelActivities() {
    m_reloading = false;
    m_reloadTimer = 0.0f;
    m_spin = 0.0f;
    m_charging = false;
    m_charge = 0.0f;
}

void ServerPlayer::StartReload() {
    const WeaponDef& def = GetWeaponDef(CurrentWeapon);
    const WeaponAmmo& ammo = m_ammo[CurrentWeapon];

    if (def.clipSize <= 0 || m_reloading) return;
    if (ammo.clip >= def.clipSize || ammo.reserve <= 0) return;

    m_reloading = true;
    m_reloadTimer = 0.0f;
}

void ServerPlayer::UpdateReload(float deltaTime) {
    if (!m_reloading) return;

    const WeaponDef& def = GetWeaponDef(CurrentWeapon);
    WeaponAmmo& ammo = m_ammo[CurrentWeapon];

    m_reloadTimer += deltaTime;

    if (def.perShellReload) {
        // Kovanlar tek tek dolar; sarjor dolunca ya da yedek bitince son bulur.
        while (m_reloading && m_reloadTimer >= def.reloadTime) {
            m_reloadTimer -= def.reloadTime;

            if (ammo.clip < def.clipSize && ammo.reserve > 0) {
                ammo.clip++;
                ammo.reserve--;
            }
            if (ammo.clip >= def.clipSize || ammo.reserve <= 0) m_reloading = false;
        }
    }
    else if (m_reloadTimer >= def.reloadTime) {
        const int take = MinI(def.clipSize - ammo.clip, ammo.reserve);
        ammo.clip += take;
        ammo.reserve -= take;
        m_reloading = false;
    }
}

void ServerPlayer::UpdateWeapon(float deltaTime, int owner, const glm::vec3& eye,
    const glm::vec3& f, const glm::vec3& r, const glm::vec3& u) {

    WeaponAmmo& ammo = m_ammo[CurrentWeapon];
    const bool ready = (m_cooldown <= 0.0f) && !m_reloading;
    const int primary = m_input.attack1Count;
    const int secondary = m_input.attack2Count;

    switch (CurrentWeapon) {
    case PISTOL:
    case DEAGLE: {
        if (primary > 0 && ready) {
            if (ammo.clip > 0) {
                const bool deagle = (CurrentWeapon == DEAGLE);
                ammo.clip--;
                Combat::FireBullets(owner, eye, f, r, u, 1,
                    deagle ? DEAGLE_SPREAD : PISTOL_SPREAD,
                    deagle ? DEAGLE_DAMAGE : PISTOL_DAMAGE,
                    deagle ? DEAGLE_HEADSHOT : 1.0f);
                m_cooldown = deagle ? DEAGLE_INTERVAL : PISTOL_INTERVAL;
            }
            else {
                StartReload(); // bos sarjorle atis denemesi otomatik dolum baslatir
            }
        }
        break;
    }

    case SHOTGUN: {
        if (ready && (primary > 0 || secondary > 0)) {
            if (ammo.clip <= 0) { StartReload(); break; }

            const bool both = secondary > 0; // sag tik: iki kovan birden
            const int shells = both ? MinI(2, ammo.clip) : 1;
            ammo.clip -= shells;
            Combat::FireBullets(owner, eye, f, r, u, SHOTGUN_PELLETS * shells,
                both ? SHOTGUN_ALT_SPREAD : SHOTGUN_SPREAD, SHOTGUN_PELLET_DAMAGE, 1.0f);
            m_cooldown = both ? SHOTGUN_ALT_INTERVAL : SHOTGUN_INTERVAL;
        }
        break;
    }

    case DOUBLE_BARREL: {
        if (ready && (primary > 0 || secondary > 0)) {
            if (ammo.clip <= 0) { StartReload(); break; }

            const bool both = secondary > 0; // sag tik: iki namlu birden
            const int shells = both ? MinI(2, ammo.clip) : 1;
            ammo.clip -= shells;
            Combat::FireBullets(owner, eye, f, r, u, DOUBLE_PELLETS * shells,
                both ? DOUBLE_ALT_SPREAD : DOUBLE_SPREAD, DOUBLE_PELLET_DAMAGE, 1.0f);
            m_cooldown = both ? DOUBLE_ALT_INTERVAL : DOUBLE_INTERVAL;
        }
        break;
    }

    case SAWBLADE: {
        // Sag tik basili tutuldukca donus artar, birakilinca yavasca soner.
        if (m_input.attack2Held && ammo.reserve > 0) {
            m_spin = MinF(1.0f, m_spin + deltaTime / SAW_SPIN_UP_TIME);
        }
        else {
            m_spin = MaxF(0.0f, m_spin - deltaTime / SAW_SPIN_DECAY_TIME);
        }

        if (primary > 0 && ready && ammo.reserve > 0) {
            const int damage = static_cast<int>(SAW_MIN_DAMAGE + (SAW_MAX_DAMAGE - SAW_MIN_DAMAGE) * m_spin);
            Combat::SpawnSawblade(owner, Combat::MuzzlePoint(eye, f, 30.0f), f, m_spin, damage);
            ammo.reserve--;
            m_spin = 0.0f;
            m_cooldown = SAW_INTERVAL;
        }
        break;
    }

    case GRENADE_LAUNCHER: {
        if (primary > 0 && ready && ammo.reserve > 0) {
            // Hafif yukari egimli firlatis; tur (el bombasi, sis, lav) rastgele secilir.
            const glm::vec3 dir = glm::normalize(f + u * 0.12f);
            Combat::SpawnBomb(owner, Combat::MuzzlePoint(eye, f, 28.0f), dir, Combat::PickRandomBombKind());
            ammo.reserve--;
            m_cooldown = BOMB_INTERVAL;
        }
        break;
    }

    case ROCKET_LAUNCHER: {
        if (primary > 0 && ready && ammo.reserve > 0) {
            Combat::SpawnRocket(owner, Combat::MuzzlePoint(eye, f, 28.0f), f);
            ammo.reserve--;
            m_cooldown = ROCKET_INTERVAL;
        }
        break;
    }

    case STAPLEGUN: {
        if (primary > 0 && ready && ammo.reserve > 0) {
            // Bes top, ayni yatay hizada yan yana ve paralel gider (acisal dagilim yok).
            const glm::vec3 muzzle = Combat::MuzzlePoint(eye, f, 30.0f) - u * 6.0f;
            const int half = STAPLE_COUNT / 2;
            for (int i = -half; i <= half; i++) {
                Combat::SpawnStapleBall(owner, muzzle + r * (static_cast<float>(i) * STAPLE_SPACING), f);
            }
            ammo.reserve--; // bir salvo bir cephane harcar
            m_cooldown = STAPLE_INTERVAL;
        }
        break;
    }

    case RAYGUN: {
        if (primary > 0 && ready && ammo.reserve > 0) {
            Combat::FireRay(owner, eye, f, RAY_DAMAGE);
            ammo.reserve--;
            m_cooldown = RAY_INTERVAL;
        }
        break;
    }

    case LASTGUN: {
        if (!m_charging) {
            if (m_input.attack1Held && ready && ammo.reserve > 0) {
                m_charging = true;
                m_charge = 0.0f;
            }
        }
        else if (!m_input.attack1Held) {
            // Erken birakma sarji iptal eder; cephane harcanmaz.
            m_charging = false;
            m_charge = 0.0f;
        }
        else {
            m_charge += deltaTime / LASTGUN_CHARGE_TIME;
            if (m_charge >= 1.0f) {
                // Sarj dolunca otomatik ates: tek devasa top.
                Combat::SpawnLastBall(owner, Combat::MuzzlePoint(eye, f, 48.0f), f);
                ammo.reserve--;
                m_charging = false;
                m_charge = 0.0f;
                m_cooldown = LASTGUN_INTERVAL;
            }
        }
        break;
    }

    default:
        break;
    }
}

void ServerPlayer::Think(float deltaTime, int ownerIndex, const glm::vec3& eye) {
    m_cooldown = MaxF(0.0f, m_cooldown - deltaTime);

    if (Health <= 0) {
        CancelActivities();
        m_input = PlayerActionCommand();
        m_fovBoost = Approach(m_fovBoost, 0.0f, FOV_FALL_RATE * deltaTime);
        return;
    }

    // Silah degistirme: devam eden dolum, donus ve sarj iptal olur.
    const int requested = m_input.switchToWeapon;
    if (requested >= 0 && requested < WEAPON_COUNT && requested != CurrentWeapon) {
        CancelActivities();
        CurrentWeapon = requested;
        m_cooldown = MaxF(m_cooldown, SWITCH_DELAY);
    }

    for (int i = 0; i < m_input.reloadCount; i++) StartReload();
    UpdateReload(deltaTime);

    glm::vec3 forward, right, up;
    ComputeAim(m_input.aimYaw, m_input.aimPitch, forward, right, up);
    UpdateWeapon(deltaTime, ownerIndex, eye, forward, right, up);

    // Lastgun sarjinda gorus acisi aniden genisler, bitince hizla geri doner.
    const float fovTarget = m_charging ? LASTGUN_FOV_BOOST : 0.0f;
    m_fovBoost = Approach(m_fovBoost, fovTarget, (m_charging ? FOV_RISE_RATE : FOV_FALL_RATE) * deltaTime);

    // Sayaclar bu tick'te tuketildi.
    m_input.attack1Count = 0;
    m_input.attack2Count = 0;
    m_input.reloadCount = 0;
    m_input.switchToWeapon = -1;
}

PlayerStats ServerPlayer::GetStats() const {
    const WeaponDef& def = GetWeaponDef(CurrentWeapon);
    const WeaponAmmo& ammo = m_ammo[CurrentWeapon];
    const bool hasClip = def.clipSize > 0;

    PlayerStats stats;
    stats.health = Health;
    stats.maxHealth = maxHealth;
    stats.armor = Armor;
    stats.maxArmor = maxArmor;
    stats.currentWeapon = CurrentWeapon;
    stats.bullets = hasClip ? ammo.clip : ammo.reserve;
    stats.maxBullets = hasClip ? def.clipSize : def.maxReserve;
    stats.reserveAmmo = hasClip ? ammo.reserve : 0;
    stats.isReloading = m_reloading;
    stats.alive = Health > 0;
    stats.charge = (CurrentWeapon == SAWBLADE) ? m_spin : (CurrentWeapon == LASTGUN ? m_charge : 0.0f);
    stats.fovBoost = m_fovBoost;
    return stats;
}

void ServerPlayer::Death() {
    // Olum davranisi (kamera, girdi kilidi, ekran yazisi) henuz tanimlanmadi.
    CancelActivities();
}