#pragma once
#include <glm/glm.hpp>

// BSPMap.h icindeki TraceResult tanimi buraya tasinir; BSPMap.h bu basligi
// include eder.
struct TraceResult {
    bool startSolid = false;
    bool allSolid = true;
    bool inOpen = false;
    bool inWater = false;
    float fraction = 1.0f;
    glm::vec3 endPos{ 0.0f };
    glm::vec3 planeNormal{ 0.0f, 1.0f, 0.0f };
};

#define WORLD_QUERY_INTERFACE_VERSION "WorldQuery001"

// Yuklu haritaya collision ve entity sorgulari. Exe (ve dedicated exe) kendi
// BSPMap'ini bu arayuzle sarip DLL'lere verir. Entity verisi std::map yerine
// duz C erisimiyle sunulur; donen pointer'lar harita degisene kadar gecerlidir.
class IWorldQuery {
public:
    virtual bool IsLoaded() const = 0;
    virtual const char* GetMapName() const = 0;

    virtual TraceResult TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex) const = 0;
    virtual glm::vec3 SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex) const = 0;
    virtual bool IsPointSolid(const glm::vec3& position, int hullIndex) const = 0;

    virtual int GetEntityCount() const = 0;
    // Anahtar yoksa nullptr doner.
    virtual const char* GetEntityKeyValue(int entityIndex, const char* key) const = 0;
};