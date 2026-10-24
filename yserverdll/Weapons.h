#pragma once
#include "playerstats.h"

// Bir silahin cephane ve dolum tanimi. Ates hizi ve hasar degerleri silahin
// davranisinin yazildigi ServerPlayer.cpp'nin basindaki sabitlerdedir.
struct WeaponDef {
    const char* name;
    int clipSize;        // 0: sarjorsuz silah; cephane dogrudan yedekten harcanir
    int startClip;
    int startReserve;
    int maxReserve;
    float reloadTime;    // tam dolum suresi; perShellReload ise kovan basina sure
    bool perShellReload;
};

inline const WeaponDef& GetWeaponDef(int weapon) {
    static const WeaponDef defs[WEAPON_COUNT] = {
        //  ad                  sarjor  baslangic  yedek  azami  dolum  tek-tek
        { "PISTOL",              15,     15,       60,   150,   1.2f,  false },
        { "DEAGLE",               7,      7,       28,    70,   1.8f,  false },
        { "SHOTGUN",              8,      8,       32,    80,   0.5f,  true  },
        { "DOUBLE BARREL",        2,      2,       24,    48,   1.7f,  false },
        { "SAWBLADE LAUNCHER",    0,      0,        6,    20,   0.0f,  false },
        { "GRENADE LAUNCHER",     0,      0,       12,    30,   0.0f,  false },
        { "ROCKET LAUNCHER",      0,      0,       10,    30,   0.0f,  false },
        { "STAPLEGUN",            0,      0,       30,    90,   0.0f,  false },
        { "RAYGUN",               0,      0,        8,    20,   0.0f,  false },
        { "LASTGUN",              0,      0,        3,     8,   0.0f,  false }
    };

    if (weapon < 0 || weapon >= WEAPON_COUNT) weapon = PISTOL;
    return defs[weapon];
}