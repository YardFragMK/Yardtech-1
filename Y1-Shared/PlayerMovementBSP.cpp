#include "PlayerMovement.h"
#include "iworldquery.h"
#include "BSPMap.h"

namespace {
    // BSPMap'i IWorldQuery arayuzune uyarlayan, yalnizca fizik icin yeterli
    // bir sarmalayici. Entity ve harita adi sorgulari bu yolda kullanilmaz.
    class BSPMapQueryAdapter : public IWorldQuery {
    public:
        explicit BSPMapQueryAdapter(const BSPMap& map) : m_map(map) {}

        bool IsLoaded() const override { return true; }
        const char* GetMapName() const override { return ""; }

        TraceResult TraceLine(const glm::vec3& start, const glm::vec3& end, int hullIndex) const override {
            return m_map.TraceLine(start, end, hullIndex);
        }
        glm::vec3 SlideMove(const glm::vec3& start, const glm::vec3& end, int hullIndex) const override {
            return m_map.SlideMove(start, end, hullIndex);
        }
        bool IsPointSolid(const glm::vec3& position, int hullIndex) const override {
            return m_map.IsPointSolid(position, hullIndex);
        }

        int GetEntityCount() const override { return 0; }
        const char* GetEntityKeyValue(int, const char*) const override { return nullptr; }

    private:
        const BSPMap& m_map;
    };
}

PlayerPhysicsEvents SimulatePlayerPhysics(
    PlayerPhysicsState& state,
    const PlayerInputCommand& input,
    BSPMap& map,
    float deltaTime,
    float gravity,
    float jumpforce)
{
    BSPMapQueryAdapter adapter(map);
    return SimulatePlayerPhysics(state, input, adapter, deltaTime, gravity, jumpforce);
}