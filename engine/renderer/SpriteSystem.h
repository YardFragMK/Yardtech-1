#pragma once
#include <windows.h>
#include <GL/gl.h>

// Effect sprites, loaded from nvs1/sprites/<name>.tga through the VFS.
// There are NO built-in or generated fallbacks: when a file is missing the sprite is
// unavailable and every effect that needs it is skipped. Animated sprites are frame
// grids (left to right, top to bottom) in a single file.
enum SpriteId {
    SPR_GLOW = 0,    // soft glow, additive
    SPR_SMOKE,       // smoke puff, alpha blended
    SPR_FIRE,        // 4x4 animated fireball (greyscale; the particle supplies the colour)
    SPR_SPARK,       // small spark
    SPR_RING,        // shockwave ring
    SPR_BUBBLE,      // liquid bubble
    SPR_BEAM,        // beam cross-section (soft across the width)
    SPR_POOL_MASK,   // irregular pool edge (alpha channel)
    SPR_LAVA,        // lava texture (tileable)
    SPR_ACID,        // acid texture (tileable)
    SPR_COUNT
};

struct SpriteInfo {
    GLuint texture = 0;   // 0: the file was not found
    int cols = 1;
    int rows = 1;
};

namespace SpriteSystem {
    // Loads the sprite on first use (a GL context is required).
    const SpriteInfo& Get(SpriteId id);
    bool Available(SpriteId id);
    const char* FileName(SpriteId id);

    // Prints which sprite files exist ("fx_status").
    void LogStatus();

    // Deletes all sprites; they are loaded again when next used ("sprite_reload").
    void Reload();
}