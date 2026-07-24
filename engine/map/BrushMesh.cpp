#include"BrushMesh.h"
#include<algorithm>
#include<iostream>

// ─────────────────────────────────────────────
// 3 duzlemin kesisim noktasini bulur (Cramer's rule / lineer sistem cozumu).
// Duzlemler paralel/dejenere ise false doner.
// ─────────────────────────────────────────────
bool BrushMeshBuilder::IntersectPlanes(const BrushFace& a, const BrushFace& b, const BrushFace& c, glm::vec3& outPoint) {
    const glm::vec3& n1 = a.normal;
    const glm::vec3& n2 = b.normal;
    const glm::vec3& n3 = c.normal;

    float denom = glm::dot(n1, glm::cross(n2, n3));
    if (std::abs(denom) < 1e-6f) {
        return false; // duzlemler paralel ya da lineer bagimli
    }

    glm::vec3 result =
        (glm::cross(n2, n3) * a.dist) +
        (glm::cross(n3, n1) * b.dist) +
        (glm::cross(n1, n2) * c.dist);

    outPoint = result / denom;
    return true;
}

// ─────────────────────────────────────────────
// Bir noktanin brush'in TUM duzlemlerinin "ic" tarafinda olup olmadigini kontrol eder.
// Half-space testi: dot(normal, point) - dist <= epsilon ise ic taraftadir.
// ─────────────────────────────────────────────
bool BrushMeshBuilder::IsPointInsideBrush(const glm::vec3& point, const Brush& brush, float epsilon) {
    for (const auto& face : brush.faces) {
        float d = glm::dot(face.normal, point) - face.dist;
        if (d > epsilon) {
            return false; // duzlemin disinda (pozitif taraf = dis)
        }
    }
    return true;
}

// ─────────────────────────────────────────────
// Bir yuzeyin vertex'lerini, o yuzeyin normali etrafinda aci sirasina gore siralar.
// Bu, convex polygon'un dogru sirayla (TRIANGLE_FAN icin) cizilmesini saglar.
// ─────────────────────────────────────────────
void BrushMeshBuilder::SortVerticesAroundNormal(std::vector<glm::vec3>& points, const glm::vec3& normal) {
    if (points.size() < 3) return;

    // Merkez nokta (centroid) hesapla
    glm::vec3 center(0.0f);
    for (auto& p : points) center += p;
    center /= (float)points.size();

    // Duzlem uzerinde iki ortogonal eksen sec (referans vektor + normal'e dik)
    glm::vec3 refDir = points[0] - center;
    if (glm::length(refDir) < 1e-6f) return;
    refDir = glm::normalize(refDir);
    glm::vec3 tangent = glm::normalize(glm::cross(normal, refDir));

    std::sort(points.begin(), points.end(), [&](const glm::vec3& p1, const glm::vec3& p2) {
        glm::vec3 d1 = p1 - center;
        glm::vec3 d2 = p2 - center;

        float angle1 = atan2(glm::dot(d1, tangent), glm::dot(d1, refDir));
        float angle2 = atan2(glm::dot(d2, tangent), glm::dot(d2, refDir));

        return angle1 < angle2;
        });
}

// ─────────────────────────────────────────────
// Valve 220 UV hesaplama: dunya-uzayi noktasini texture eksenlerine izdusurur.
// ─────────────────────────────────────────────
glm::vec2 BrushMeshBuilder::CalcUV(const glm::vec3& p, const BrushFace& face, int texW, int texH) {
    float u = glm::dot(p, face.uAxis) / std::max(face.scaleX, 0.0001f) + face.uOffset;
    float v = glm::dot(p, face.vAxis) / std::max(face.scaleY, 0.0001f) + face.vOffset;

    if (texW > 0) u /= (float)texW;
    if (texH > 0) v /= (float)texH;

    return glm::vec2(u, v);
}

// ─────────────────────────────────────────────
// Ana fonksiyon: brush'i mesh'e cevirir
// ─────────────────────────────────────────────
bool BrushMeshBuilder::Build(const Brush& brush, BrushMesh& outMesh) {
    outMesh.faces.clear();

    size_t faceCount = brush.faces.size();
    if (faceCount < 4) {
        return false; // convex bir hacim icin en az 4 duzlem gerekir (tetrahedron)
    }

    for (size_t i = 0; i < faceCount; i++) {
        const BrushFace& face = brush.faces[i];

        std::vector<glm::vec3> facePoints;

        // Bu yuzeyi diger her iki yuzeyle kesistir
        for (size_t j = 0; j < faceCount; j++) {
            if (j == i) continue;
            for (size_t k = j + 1; k < faceCount; k++) {
                if (k == i) continue;

                glm::vec3 point;
                if (!IntersectPlanes(face, brush.faces[j], brush.faces[k], point)) {
                    continue; // paralel duzlemler, gecersiz kesisim
                }

                if (!IsPointInsideBrush(point, brush)) {
                    continue; // brush'in disinda kalan bir kesisim
                }

                // Ayni noktanin tekrar eklenmesini onle (farkli j,k ciftleri
                // ayni kesisim noktasini birden fazla verebilir)
                bool duplicate = false;
                for (const auto& existing : facePoints) {
                    if (glm::distance(existing, point) < 0.01f) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate) {
                    facePoints.push_back(point);
                }
            }
        }

        if (facePoints.size() < 3) {
            continue; // bu yuzey brush'ta hic gorunmuyor (baska yuzeyler tarafindan tamamen kapli)
        }

        SortVerticesAroundNormal(facePoints, face.normal);

        // Texture boyutlarini simdilik 1x1 varsayiyoruz; gercek texture
        // yuklendiginde bu deger BrushMeshBuilder disaridan alinacak
        // (asama: texture sistemi entegrasyonu)
        int texW = 1, texH = 1;

        MeshFace meshFace;
        meshFace.textureName = face.textureName;

        for (const auto& p : facePoints) {
            MeshVertex mv;
            mv.position = ToEngineSpace(p);
            mv.uv = CalcUV(p, face, texW, texH);
            mv.normal = ToEngineSpace(face.normal); // normal de ayni eksen donusumunu alir
            meshFace.vertices.push_back(mv);
        }

        outMesh.faces.push_back(std::move(meshFace));
    }

    return !outMesh.faces.empty();
}