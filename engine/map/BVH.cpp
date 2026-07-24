#include"BVH.h"
#include<algorithm>
#include<numeric>

BVHBounds BVH::ComputeBounds(const std::vector<int>& indices) const {
    BVHBounds b;
    for (int idx : indices) {
        b.Encapsulate(m_primitives[idx].bounds);
    }
    return b;
}

void BVH::Build(std::vector<BVHPrimitive> primitives) {
    m_primitives = std::move(primitives);

    std::vector<int> indices(m_primitives.size());
    std::iota(indices.begin(), indices.end(), 0);

    m_root = BuildRecursive(indices, 0);
}

std::unique_ptr<BVHNode> BVH::BuildRecursive(std::vector<int>& indices, int depth) {
    auto node = std::make_unique<BVHNode>();
    node->bounds = ComputeBounds(indices);

    if ((int)indices.size() <= MAX_LEAF_PRIMITIVES || depth >= MAX_DEPTH) {
        node->primitiveIndices = indices;
        return node;
    }

    // En uzun ekseni bul (split ekseni)
    glm::vec3 extent = node->bounds.Extent();
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z) axis = 1;
    else if (extent.z > extent.x && extent.z > extent.y) axis = 2;

    // Primitive'leri o eksendeki merkez konumuna gore sirala (median split)
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        float ca = m_primitives[a].bounds.Center()[axis];
        float cb = m_primitives[b].bounds.Center()[axis];
        return ca < cb;
        });

    size_t mid = indices.size() / 2;
    std::vector<int> leftIndices(indices.begin(), indices.begin() + mid);
    std::vector<int> rightIndices(indices.begin() + mid, indices.end());

    // Dejenere bolunme kontrolu (hepsi ayni tarafa dusuyorsa sonsuz recursion'i onle)
    if (leftIndices.empty() || rightIndices.empty()) {
        node->primitiveIndices = indices;
        return node;
    }

    node->left = BuildRecursive(leftIndices, depth + 1);
    node->right = BuildRecursive(rightIndices, depth + 1);

    return node;
}

void BVH::QueryFrustum(const Frustum& frustum, std::vector<int>& outIndices) const {
    outIndices.clear();
    if (m_root) {
        QueryFrustumRecursive(m_root.get(), frustum, outIndices);
    }
}

void BVH::QueryFrustumRecursive(const BVHNode* node, const Frustum& frustum, std::vector<int>& outIndices) const {
    if (!node || !frustum.IsBoxVisible(node->bounds)) {
        return; // bu dal tamamen frustum disinda, tum alt agaci atla
    }

    if (node->IsLeaf()) {
        for (int idx : node->primitiveIndices) {
            outIndices.push_back(idx);
        }
        return;
    }

    QueryFrustumRecursive(node->left.get(), frustum, outIndices);
    QueryFrustumRecursive(node->right.get(), frustum, outIndices);
}

void BVH::QueryBounds(const BVHBounds& queryBounds, std::vector<int>& outIndices) const {
    outIndices.clear();
    if (m_root) {
        QueryBoundsRecursive(m_root.get(), queryBounds, outIndices);
    }
}

void BVH::QueryBoundsRecursive(const BVHNode* node, const BVHBounds& queryBounds, std::vector<int>& outIndices) const {
    if (!node || !node->bounds.Intersects(queryBounds)) {
        return;
    }

    if (node->IsLeaf()) {
        for (int idx : node->primitiveIndices) {
            if (m_primitives[idx].bounds.Intersects(queryBounds)) {
                outIndices.push_back(idx);
            }
        }
        return;
    }

    QueryBoundsRecursive(node->left.get(), queryBounds, outIndices);
    QueryBoundsRecursive(node->right.get(), queryBounds, outIndices);
}