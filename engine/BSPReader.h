#pragma once
#include <cstdint>
#include <string>

//---------------------------------------------------------
//(.bsp version 30) header / lump yapisi
//---------------------------------------------------------

#pragma pack(push, 1)
struct BSPLump
{
    int32_t offset;
    int32_t length;
};

struct BSPHeader
{
    int32_t version;    //  30
    BSPLump  lumps[15]; // LUMP_ENTITIES = 0
};
#pragma pack(pop)

constexpr int LUMP_ENTITIES = 0;

// .bsp dosyasindan sadece entity lump'ini okur ve ham metin olarak dondurur.
// Basarisiz olursa bos string dondurur.
std::string ReadEntityLump(const std::string& bspPath);