#pragma once
#include"MapTypes.h"
#include<glm/glm.hpp>
#include<vector>
#include<string>

struct MeshVertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 baseColor{ 1.0f, 1.0f, 1.0f }; // statik isik (VertexLighting'den, bir kere hesaplanir)
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };     // render'da kullanilan nihai renk (baseColor + dynamic)
};

struct MeshFace {
    std::vector<MeshVertex> vertices; // convex polygon, TRIANGLE_FAN ile cizilir
    std::string textureName;
};

struct BrushMesh {
    std::vector<MeshFace> faces;
};

class BrushMeshBuilder {
public:
    // Tek bir brush'i mesh'e cevirir. Basarisiz olursa (dejenere brush) false doner.
    static bool Build(const Brush& brush, BrushMesh& outMesh);

private:
    static bool IntersectPlanes(const BrushFace& a, const BrushFace& b, const BrushFace& c, glm::vec3& outPoint);
    static bool IsPointInsideBrush(const glm::vec3& point, const Brush& brush, float epsilon = 0.01f);
    static void SortVerticesAroundNormal(std::vector<glm::vec3>& points, const glm::vec3& normal);
    static glm::vec2 CalcUV(const glm::vec3& bspSpacePoint, const BrushFace& face, int texW, int texH);

    // BSP Z-up -> motorun Y-up sistemine cevirir
    static glm::vec3 ToEngineSpace(const glm::vec3& mapSpacePoint) {
        return glm::vec3(mapSpacePoint.x, mapSpacePoint.z, -mapSpacePoint.y);
    }
};