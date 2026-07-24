#pragma once
#include"BrushMesh.h"
#include<glm/glm.hpp>
#include<vector>
#include<memory>
#include<cfloat>

struct BVHBounds {
    glm::vec3 min{ FLT_MAX };
    glm::vec3 max{ -FLT_MAX };

    void Encapsulate(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    void Encapsulate(const BVHBounds& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
    glm::vec3 Center() const { return (min + max) * 0.5f; }
    glm::vec3 Extent() const { return (max - min) * 0.5f; }

    bool Intersects(const BVHBounds& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool Contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z;
    }
};

// Frustum, 6 duzlemle tanimlanir (near, far, left, right, top, bottom).
// Her duzlem: normal + dist, ic taraf normal yonundedir.
struct FrustumPlane {
    glm::vec3 normal;
    float dist;
};
struct Frustum {
    FrustumPlane planes[6];

    // BVHBounds tamamen/kismen frustum disinda mi? (basit AABB-vs-frustum testi)
    bool IsBoxVisible(const BVHBounds& box) const {
        for (const auto& p : planes) {
            glm::vec3 positiveVertex(
                p.normal.x >= 0 ? box.max.x : box.min.x,
                p.normal.y >= 0 ? box.max.y : box.min.y,
                p.normal.z >= 0 ? box.max.z : box.min.z
            );
            if (glm::dot(p.normal, positiveVertex) + p.dist < 0.0f) {
                return false; // kutu tamamen bu duzlemin disinda
            }
        }
        return true;
    }
};

// Her brush'in mesh'i + onceden hesaplanmis bounding box'i
struct BVHPrimitive {
    BrushMesh mesh;
    BVHBounds bounds;
};

struct BVHNode {
    BVHBounds bounds;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;

    // Sadece yaprak (leaf) dugumlerde dolu olur: bu dugumdeki primitive indeksleri
    std::vector<int> primitiveIndices;

    bool IsLeaf() const { return !left && !right; }
};

class BVH {
public:
    void Build(std::vector<BVHPrimitive> primitives);

    // Frustum icinde gorunen primitive indekslerini outIndices'e ekler
    void QueryFrustum(const Frustum& frustum, std::vector<int>& outIndices) const;

    // Bir AABB ile kesisen primitive indekslerini outIndices'e ekler (collision icin)
    void QueryBounds(const BVHBounds& queryBounds, std::vector<int>& outIndices) const;

    const std::vector<BVHPrimitive>& GetPrimitives() const { return m_primitives; }
    std::vector<BVHPrimitive>& GetPrimitivesMutable() { return m_primitives; }

private:
    std::unique_ptr<BVHNode> BuildRecursive(std::vector<int>& indices, int depth);
    BVHBounds ComputeBounds(const std::vector<int>& indices) const;

    void QueryFrustumRecursive(const BVHNode* node, const Frustum& frustum, std::vector<int>& outIndices) const;
    void QueryBoundsRecursive(const BVHNode* node, const BVHBounds& queryBounds, std::vector<int>& outIndices) const;

    std::vector<BVHPrimitive> m_primitives;
    std::unique_ptr<BVHNode> m_root;

    static constexpr int MAX_LEAF_PRIMITIVES = 4; // bu sayidan az kalinca yaprak yap
    static constexpr int MAX_DEPTH = 24;
};