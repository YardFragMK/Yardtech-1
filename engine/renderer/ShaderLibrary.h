#pragma once
#include <string>
#include "ShaderProgram.h"

// Adla erisilen shader programlari. "ad" icin shader/<ad>.frag ve
// shader/<vertAd>.vert dosyalari VFS'ten okunur (vertAd bos ise <ad>.vert).
// Boylece birden fazla fragment shader ortak bir vertex shader'i paylasabilir.
namespace ShaderLibrary {
    bool Load(const std::string& name, const std::string& vertName = "");

    // Yuklenmemis ya da hatali programlar icin nullptr doner. Isaretci, yeniden
    // yuklemelerde (shader_reload) gecerli kalir; yine de her karede sorgulanmasi onerilir.
    ShaderProgram* Get(const std::string& name);

    void ReloadAll();

    // Konsola "shader_reload" komutunu ekler.
    void RegisterCommands();
}