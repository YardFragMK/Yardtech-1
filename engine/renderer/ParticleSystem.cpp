#include "ParticleSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <windows.h>
#include <GL/gl.h>
#include "../GLExtensions.h"

namespace {
    constexpr size_t MAX_PARTICLES = 12000;   // was 4096; dense fog clouds need many puffs

    std::vector<Particle> s_particles;

    struct DrawItem {
        const Particle* particle;
        float distanceSq;
    };

    float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

    void EmitQuad(const Particle& p, const glm::vec3& right, const glm::vec3& up, float size, const SpriteInfo& sprite, int frame) {
        const float h = size * 0.5f;
        const float c = std::cos(p.rot), s = std::sin(p.rot);
        const glm::vec3 rx = (right * c + up * s) * h;
        const glm::vec3 ry = (up * c - right * s) * h;

        const int col = frame % sprite.cols;
        const int row = frame / sprite.cols;
        const float u0 = static_cast<float>(col) / static_cast<float>(sprite.cols);
        const float u1 = static_cast<float>(col + 1) / static_cast<float>(sprite.cols);
        const float v0 = static_cast<float>(row) / static_cast<float>(sprite.rows);
        const float v1 = static_cast<float>(row + 1) / static_cast<float>(sprite.rows);

        // Gorselin ustu v0'dadir; dortgenin ustu +ry yonudur.
        const glm::vec3 a = p.pos - rx + ry, b = p.pos + rx + ry, d = p.pos + rx - ry, e = p.pos - rx - ry;
        glTexCoord2f(u0, v0); glVertex3f(a.x, a.y, a.z);
        glTexCoord2f(u1, v0); glVertex3f(b.x, b.y, b.z);
        glTexCoord2f(u1, v1); glVertex3f(d.x, d.y, d.z);
        glTexCoord2f(u0, v1); glVertex3f(e.x, e.y, e.z);
    }

    void DrawList(const std::vector<DrawItem>& list, bool additive, const glm::vec3& camPos,
        const glm::vec3& right, const glm::vec3& up) {
        glBlendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

        GLuint lastTexture = 0;
        bool open = false;

        for (const DrawItem& item : list) {
            const Particle& p = *item.particle;
            const float t = Clamp01(p.age / p.life);

            glm::vec4 color = p.color0 + (p.color1 - p.color0) * t;
            if (p.fadeIn > 0.0f) color.a *= Clamp01(t / p.fadeIn);
            if (p.nearFade > 0.0f) {
                const float distance = std::sqrt(item.distanceSq);
                color.a *= Clamp01((distance - p.nearFade * 0.3f) / (p.nearFade * 0.7f));
            }
            if (color.a <= 0.002f) continue;

            const SpriteInfo& sprite = SpriteSystem::Get(p.sprite);
            if (sprite.texture == 0) continue;
            if (sprite.texture != lastTexture) {
                if (open) glEnd();
                glBindTexture(GL_TEXTURE_2D, sprite.texture);
                glBegin(GL_QUADS);
                open = true;
                lastTexture = sprite.texture;
            }

            const int frames = sprite.cols * sprite.rows;
            const int frame = p.animate ? std::min<>(frames - 1, static_cast<int>(t * static_cast<float>(frames))) : (p.frame % frames);

            glColor4f(color.r, color.g, color.b, color.a);
            EmitQuad(p, right, up, Lerp(p.size0, p.size1, t), sprite, frame);
        }
        if (open) glEnd();
    }
}

void ParticleSystem::Spawn(const Particle& particle) {
    if (s_particles.size() >= MAX_PARTICLES) return;

    // No sprite file means no particle: there is no built-in default look.
    if (!SpriteSystem::Available(particle.sprite)) return;

    s_particles.push_back(particle);
}

void ParticleSystem::Update(float dt) {
    for (size_t i = 0; i < s_particles.size();) {
        Particle& p = s_particles[i];
        p.age += dt;

        if (p.age >= p.life) {
            p = s_particles.back();
            s_particles.pop_back();
            continue;
        }

        p.vel += p.accel * dt;
        const float damping = 1.0f - p.drag * dt;
        p.vel *= (damping > 0.0f ? damping : 0.0f);
        p.pos += p.vel * dt;
        p.rot += p.rotVel * dt;
        ++i;
    }
}

void ParticleSystem::Render(const glm::vec3& camPos, const glm::vec3& right, const glm::vec3& up) {
    if (s_particles.empty()) return;

    std::vector<DrawItem> alphaList, additiveList;
    alphaList.reserve(s_particles.size());
    additiveList.reserve(s_particles.size());

    for (const Particle& p : s_particles) {
        const glm::vec3 d = p.pos - camPos;
        DrawItem item = { &p, glm::dot(d, d) };
        (p.additive ? additiveList : alphaList).push_back(item);
    }

    // Alfa karisimli parcaciklar arkadan one dogru siralanir; ek karisim sira gerektirmez.
    std::sort(alphaList.begin(), alphaList.end(),
        [](const DrawItem& a, const DrawItem& b) { return a.distanceSq > b.distanceSq; });

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    if (glActiveTexture_) {
        glActiveTexture_(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture_(GL_TEXTURE0);
    }
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    DrawList(alphaList, false, camPos, right, up);
    DrawList(additiveList, true, camPos, right, up);

    glPopAttrib();
}

bool ParticleSystem::Empty() { return s_particles.empty(); }
void ParticleSystem::Clear() { s_particles.clear(); }