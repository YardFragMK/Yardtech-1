#pragma once
#include <cstdint>

// GoldSrc .bsp version 30 ham binary yapilari
// (BSPReader.h'daki BSPHeader/BSPLump ile ayni offset/entity lump kullaniyor,
//  burada tam lump setini tanimliyoruz)

#pragma pack(push, 1)

enum BSPLumpIndex {
    LUMP_ENTITIES_F = 0,
    LUMP_PLANES = 1,
    LUMP_TEXTURES = 2,
    LUMP_VERTEXES = 3,
    LUMP_VISIBILITY = 4,
    LUMP_NODES = 5,
    LUMP_TEXINFO = 6,
    LUMP_FACES = 7,
    LUMP_LIGHTING = 8,
    LUMP_CLIPNODES = 9,
    LUMP_LEAVES = 10,
    LUMP_MARKSURFACES = 11,
    LUMP_EDGES = 12,
    LUMP_SURFEDGES = 13,
    LUMP_MODELS = 14,
    HEADER_LUMPS_F = 15
};

struct BSPPlane_t {
    float normal[3];
    float dist;
    int32_t type;
};

struct BSPMiptex_t {
    char name[16];
    uint32_t width;
    uint32_t height;
    uint32_t offsets[4]; // 0 ise texture bsp icinde gomulu degil, wad'dan aranir
};

struct BSPVertex_t {
    float point[3];
};

struct BSPTexInfo_t {
    float vecs[2][4]; // [0]=S ekseni (xyz+offset) [1]=T ekseni
    int32_t miptex;   // TEXTURES lump icindeki index
    int32_t flags;
};

struct BSPFace_t {
    uint16_t planenum;
    int16_t  side;
    int32_t  firstedge;
    int16_t  numedges;
    int16_t  texinfo;
    uint8_t  styles[4];
    int32_t  lightofs;
};

struct BSPEdge_t {
    uint16_t v[2]; // vertex index'leri
};

typedef int32_t BSPSurfEdge_t; // pozitifse edge.v[0]->v[1], negatifse ters

struct BSPModel_t {
    float mins[3];
    float maxs[3];
    float origin[3];
    int32_t headnode[4];
    int32_t visleafs;
    int32_t firstface;
    int32_t numfaces;
};

#pragma pack(pop)