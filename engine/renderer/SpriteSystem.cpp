#include "SpriteSystem.h"
#include "../ConVarRegistry.h"
#include "../console/Console.h"
#include "TGALoader.h"
#include "VFSypak.h"
#include "../GLExtensions.h"
#include <string>
#include <vector>

extern VirtualFileSystem vfs;

namespace {

    // Bir sprite dosyasinin duzenini anlatir. Sira SpriteId ile ayni olmalidir.
    struct SpriteDefinition {
        const char* fileName;
        int columns;     // animasyon kare izgarasinin sutun sayisi (tek kare icin 1)
        int rows;        // satir sayisi
        bool tiles;      // true: doku tekrar eder (sivi dokulari); false: kenarlara yapisir
    };

    const SpriteDefinition kDefinitions[SPR_COUNT] = {
        { "glow.tga",     1, 1, false },
        { "smoke.tga",    1, 1, false },
        { "fire.tga",     4, 4, false },   // 4x4 = 16 kareli ates animasyonu
        { "spark.tga",    1, 1, false },
        { "ring.tga",     1, 1, false },
        { "bubble.tga",   1, 1, false },
        { "beam.tga",     1, 1, false },
        { "poolmask.tga", 1, 1, false },
        { "lava.tga",     1, 1, true  },
        { "acid.tga",     1, 1, true  }
    };

    // Yukleme sonucu: "dosya yok" ile "dosya var ama okunamadi" ayri ayri bildirilir.
    enum class LoadState { NotLoaded, Loaded, FileMissing, DecodeFailed };

    SpriteInfo g_sprites[SPR_COUNT];
    LoadState g_states[SPR_COUNT] = {};
    bool g_reloadCommandRegistered = false;

    std::string VirtualPathOf(SpriteId id) {
        return std::string("sprites/") + kDefinitions[id].fileName;
    }

    GLuint UploadTexture(const std::vector<uint8_t>& rgbaPixels, int width, int height, bool tiles) {
        const GLint wrapMode = tiles ? GL_REPEAT : GL_CLAMP_TO_EDGE;

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    // Bir sprite'i VFS'ten okur. Sonuc g_states'e yazilir; basarisizsa bos SpriteInfo doner.
    SpriteInfo LoadSprite(SpriteId id) {
        const SpriteDefinition& definition = kDefinitions[id];
        const std::string path = VirtualPathOf(id);

        const std::vector<uint8_t> file = vfs.ReadFile(path);
        if (file.empty()) {
            g_states[id] = LoadState::FileMissing;
            Console::Log("Sprite dosyasi bulunamadi: " + path + " (bunu kullanan efektler kapali)");
            return SpriteInfo();
        }

        std::vector<uint8_t> pixels;
        int width = 0;
        int height = 0;
        if (!LoadTGAFromMemory(file, pixels, width, height)) {
            g_states[id] = LoadState::DecodeFailed;
            Console::Log("Sprite dosyasi OKUNAMADI: " + path + " (dosya var ama TGA bicimi desteklenmiyor olabilir; "
                "sikistirilmamis 24/32 bit TGA kullanin)");
            return SpriteInfo();
        }

        g_states[id] = LoadState::Loaded;

        SpriteInfo info;
        info.cols = definition.columns;
        info.rows = definition.rows;
        info.texture = UploadTexture(pixels, width, height, definition.tiles);
        return info;
    }

    void RegisterReloadCommandOnce() {
        if (g_reloadCommandRegistered) return;
        g_reloadCommandRegistered = true;

        ConVarRegistry::RegisterCommand("sprite_reload",
            [](int, const char* const*) { SpriteSystem::Reload(); },
            "Efekt sprite'larini nvs1/sprites klasorunden yeniden yukler", CVAR_NONE);
    }

    const char* StateText(LoadState state) {
        switch (state) {
        case LoadState::Loaded:       return "var";
        case LoadState::FileMissing:  return "YOK";
        case LoadState::DecodeFailed: return "OKUNAMADI (bicim)";
        default:                      return "?";
        }
    }

}  // namespace

const SpriteInfo& SpriteSystem::Get(SpriteId id) {
    RegisterReloadCommandOnce();

    if (id < 0 || id >= SPR_COUNT) id = SPR_GLOW;

    if (g_states[id] == LoadState::NotLoaded) {
        g_sprites[id] = LoadSprite(id);
    }
    return g_sprites[id];
}

bool SpriteSystem::Available(SpriteId id) {
    return Get(id).texture != 0;
}

const char* SpriteSystem::FileName(SpriteId id) {
    if (id < 0 || id >= SPR_COUNT) return "";
    return kDefinitions[id].fileName;
}

void SpriteSystem::LogStatus() {
    for (int i = 0; i < SPR_COUNT; ++i) {
        const SpriteId id = static_cast<SpriteId>(i);
        Get(id);   // durumun belli olmasi icin yuklemeyi dene
        Console::Log(std::string("sprite ") + FileName(id) + ": " + StateText(g_states[i]));
    }
}

void SpriteSystem::Reload() {
    for (int i = 0; i < SPR_COUNT; ++i) {
        if (g_sprites[i].texture != 0) glDeleteTextures(1, &g_sprites[i].texture);

        g_sprites[i] = SpriteInfo();
        g_states[i] = LoadState::NotLoaded;
    }
    Console::Log("Sprite'lar bir sonraki kullanimda yeniden yuklenecek.");
}