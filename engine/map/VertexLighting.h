#pragma once
#include"BrushMesh.h"
#include"MapTypes.h"
#include<glm/glm.hpp>
#include<vector>

struct SceneLight {
    glm::vec3 position;   // motor uzayinda (Y-up, zaten cevrilmis)
    float brightness;     // Quake/HL "light" key degeri, genelde 200-300 araligi
    glm::vec3 color{ 1.0f, 1.0f, 1.0f }; // renkli isik istersen (Valve "_color" key'i icin)
};

class VertexLighting {
public:
    // MapData icindeki Light entity'lerini SceneLight listesine cevirir.
    static std::vector<SceneLight> ExtractLights(const MapData& mapData);

    // Bir BrushMesh'in tum vertex'lerine isik hesaplayip yazar.
    static void ApplyLighting(BrushMesh& mesh, const std::vector<SceneLight>& lights,
        float ambient = 0.15f);

private:
    static glm::vec3 ComputeVertexColor(const glm::vec3& worldPos, const glm::vec3& normal,
        const std::vector<SceneLight>& lights, float ambient);

    // BSP Z-up -> motorun Y-up sistemine cevirir (MapEntity origin'i icin)
    static glm::vec3 ToEngineSpace(const glm::vec3& mapSpacePoint) {
        return glm::vec3(mapSpacePoint.x, mapSpacePoint.z, -mapSpacePoint.y);
    }
};