#include"VertexLighting.h"
#include<algorithm>

std::vector<SceneLight> VertexLighting::ExtractLights(const MapData& mapData) {
    std::vector<SceneLight> lights;

    for (const auto& ent : mapData.entities) {
        if (ent.kind != EntityKind::Light) continue;

        SceneLight light;
        light.position = ToEngineSpace(ent.GetOrigin());
        light.color = glm::vec3(1.0f); // varsayilan beyaz
        light.brightness = 300.0f;

        // Once "light" key'ini kontrol et: bazen "300" (tek sayi),
        // bazen "255 0 0 300" (r g b intensity) formatinda olabilir
        std::string lightStr = ent.GetString("light", "");
        if (!lightStr.empty()) {
            float a = 0, b = 0, c = 0, d = 0;
            int count = sscanf_s(lightStr.c_str(), "%f %f %f %f", &a, &b, &c, &d);

            if (count == 1) {
                light.brightness = a; // sadece parlaklik, renk beyaz kalir
            }
            else if (count == 4) {
                // r g b intensity formati
                light.color = glm::vec3(a, b, c) / 255.0f;
                light.brightness = d;
            }
        }

        // Ayrica ayri bir "_color" key'i varsa (Valve/Half-Life tarzi), onu ust gecerli yap
        std::string colorStr = ent.GetString("_color", ent.GetString("color", ""));
        if (!colorStr.empty()) {
            float r = 255.0f, g = 255.0f, b = 255.0f;
            if (sscanf_s(colorStr.c_str(), "%f %f %f", &r, &g, &b) == 3) {
                light.color = glm::vec3(r, g, b) / 255.0f;
            }
        }

        lights.push_back(light);
    }

    return lights;
}

glm::vec3 VertexLighting::ComputeVertexColor(const glm::vec3& worldPos, const glm::vec3& normal,
    const std::vector<SceneLight>& lights, float ambient) {
    glm::vec3 total(ambient); // taban aydinlik (tamamen karanlik olmasin)

    for (const auto& light : lights) {
        glm::vec3 toLight = light.position - worldPos;
        float dist = glm::length(toLight);
        if (dist < 0.001f) continue;

        glm::vec3 lightDir = toLight / dist;

        // Lambert: yuzeyin isiga ne kadar baktigi
        float ndotl = std::max(glm::dot(normal, lightDir), 0.0f);
        if (ndotl <= 0.0f) continue; // isiga arkasini donmus yuzey, katki yok

        // Mesafe azalmasi: Quake/HL tarzi yumusatilmis ters-kare
        // (brightness degeri buyuk oldugu icin - genelde 200-300 - dogrudan
        // ters kare kullanirsak cok hizli sonecek; olcekli bir formul kullaniyoruz)
        float attenuation = light.brightness / (light.brightness + dist * dist * 0.01f);
        attenuation = std::clamp(attenuation, 0.0f, 1.0f);

        total += light.color * ndotl * attenuation;
    }

    return glm::clamp(total, glm::vec3(0.0f), glm::vec3(1.0f));
}

void VertexLighting::ApplyLighting(BrushMesh& mesh, const std::vector<SceneLight>& lights, float ambient) {
    for (auto& face : mesh.faces) {
        for (auto& vertex : face.vertices) {
            vertex.baseColor = ComputeVertexColor(vertex.position, vertex.normal, lights, ambient);
            vertex.color = vertex.baseColor; // dynamic light yoksa nihai renk = base
        }
    }
}