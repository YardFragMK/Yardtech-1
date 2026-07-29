#include "BSPReader.h"
#include <fstream>
#include <cstdio>
#include"console/Console.h"

std::string ReadEntityLump(const std::string& bspPath){
    std::ifstream file(bspPath, std::ios::binary);
    if (!file) {
        Console::Log("WARNING-> bsp acilamadi");
        return "";
    }

    BSPHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(BSPHeader));

    if (header.version != 30) {
        Console::Log("WARNING-> bsp versiyonu uyumlu degil" + header.version);
    }

    const BSPLump& entLump = header.lumps[LUMP_ENTITIES];

    std::string entityData(static_cast<size_t>(entLump.length), '\0');
    file.seekg(entLump.offset, std::ios::beg);
    file.read(entityData.data(), entLump.length);

    return entityData; // "{ \"key\" \"value\" ... } { ... } ..."
}