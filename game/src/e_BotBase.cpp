#include "e_BotBase.h"
#include "../../engine/RTInstance.h"

void e_BotBase::RenderModelOrBox() const {
    if (m_model.IsLoaded()) {
        m_model.Render(); // olcek parametresi Render'a tasindi
        return;
    }

    constexpr float hx = 16.0f, hy = 32.0f, hz = 16.0f;
    glm::vec3 verts[8] = {
        {position.x - hx,position.y - hy,position.z - hz}, {position.x + hx,position.y - hy,position.z - hz},
        {position.x + hx,position.y + hy,position.z - hz}, {position.x - hx,position.y + hy,position.z - hz},
        {position.x - hx,position.y - hy,position.z + hz}, {position.x + hx,position.y - hy,position.z + hz},
        {position.x + hx,position.y + hy,position.z + hz}, {position.x - hx,position.y + hy,position.z + hz},
    };

    static const uint32_t boxIndices[36] = {
        0,1,2, 0,2,3,  4,6,5, 4,7,6,  0,4,5, 0,5,1,
        2,6,7, 2,7,3,  1,5,6, 1,6,2,  0,3,7, 0,7,4
    };

    RgPrimitiveVertex v[8] = {};
    RgColor4DPacked32 color = rgUtilPackColorFloat4D(0.35f, 0.05f, 0.05f, 1.0f);
    for (int i = 0; i < 8; i++) {
        v[i].position[0] = verts[i].x; v[i].position[1] = verts[i].y; v[i].position[2] = verts[i].z;
        v[i].color = color;
    }

    RgTransform transform{};
    transform.matrix[0][0] = 1.0f; transform.matrix[1][1] = 1.0f; transform.matrix[2][2] = 1.0f;

    RgMeshInfo mesh{};
    mesh.uniqueObjectID = reinterpret_cast<uintptr_t>(this) & 0xFFFFFFFFu;
    mesh.pMeshName = "bot_placeholder_box";
    mesh.transform = transform;

    RgMeshPrimitiveInfo prim{};
    prim.pPrimitiveNameInMesh = "box";
    prim.pVertices = v;
    prim.vertexCount = 8;
    prim.pIndices = boxIndices;
    prim.indexCount = 36;
    prim.color = color;

    rgUploadMeshPrimitive(GetRTInstance(), &mesh, &prim);
}