#include "WeaponEffects.h"
#include <SDL.h>
#include <windows.h>
#include <GL/gl.h>
#include <cmath>
#include <glm/glm.hpp>

namespace {
    constexpr float PI_F = 3.14159265f;

    float ClampF(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    float MinF2(float a, float b) { return a < b ? a : b; }

    // Yerel +X eksenini (yaw, pitch) yonune cevirir (sunucudaki yon kuraliyla ayni).
    void ApplyDirection(float yaw, float pitch) {
        glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
        glRotatef(pitch, 0.0f, 0.0f, 1.0f);
    }

    void DrawBox(float hx, float hy, float hz) {
        glBegin(GL_QUADS);
        glVertex3f(hx, -hy, -hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, hy, -hz);
        glVertex3f(-hx, -hy, hz); glVertex3f(-hx, -hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, hy, hz);
        glVertex3f(-hx, hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(hx, hy, hz); glVertex3f(-hx, hy, hz);
        glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, -hy, -hz); glVertex3f(-hx, -hy, -hz);
        glVertex3f(-hx, -hy, hz); glVertex3f(-hx, hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, -hy, hz);
        glVertex3f(hx, -hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, -hy, -hz);
        glEnd();
    }

    void DrawSphere(float radius, int slices, int stacks) {
        for (int i = 0; i < stacks; i++) {
            const float lat0 = PI_F * (-0.5f + static_cast<float>(i) / static_cast<float>(stacks));
            const float lat1 = PI_F * (-0.5f + static_cast<float>(i + 1) / static_cast<float>(stacks));
            const float y0 = std::sin(lat0), r0 = std::cos(lat0);
            const float y1 = std::sin(lat1), r1 = std::cos(lat1);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= slices; j++) {
                const float lng = 2.0f * PI_F * static_cast<float>(j) / static_cast<float>(slices);
                const float x = std::cos(lng);
                const float z = std::sin(lng);
                glVertex3f(radius * x * r0, radius * y0, radius * z * r0);
                glVertex3f(radius * x * r1, radius * y1, radius * z * r1);
            }
            glEnd();
        }
    }

    // XZ duzleminde disli cark (testere): dis ve cukur yaricaplari dönüşümlü.
    void DrawGear(float outer, float inner, int teeth) {
        const int points = teeth * 2;
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= points; i++) {
            const float a = 2.0f * PI_F * static_cast<float>(i) / static_cast<float>(points);
            const float r = (i % 2 == 0) ? outer : inner;
            glVertex3f(std::cos(a) * r, 0.0f, std::sin(a) * r);
        }
        glEnd();
    }

    void DrawDisc(float radius, int segments) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= segments; i++) {
            const float a = 2.0f * PI_F * static_cast<float>(i) / static_cast<float>(segments);
            glVertex3f(std::cos(a) * radius, 0.0f, std::sin(a) * radius);
        }
        glEnd();
    }

    // Saydam olmayan govdeler.
    void RenderSolid(const NetEntityState& e, float time) {
        switch (e.classId) {
        case ENT_CLASS_ROCKET:
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            ApplyDirection(e.yaw, e.pitch);
            glColor4f(0.55f, 0.57f, 0.62f, 1.0f);
            DrawBox(16.0f, 4.0f, 4.0f);
            glColor4f(0.9f, 0.25f, 0.1f, 1.0f);
            glTranslatef(16.0f, 0.0f, 0.0f);
            DrawBox(4.0f, 3.0f, 3.0f);
            glPopMatrix();
            break;

        case ENT_CLASS_GRENADE:
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.2f, 0.35f, 0.15f, 1.0f);
            DrawBox(5.0f, 5.0f, 5.0f);
            glPopMatrix();
            break;

        case ENT_CLASS_FOG_BOMB:
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.6f, 0.62f, 0.66f, 1.0f);
            DrawSphere(6.0f, 12, 8);
            glPopMatrix();
            break;

        case ENT_CLASS_LAVA_BOMB:
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(1.0f, 0.4f, 0.05f, 1.0f);
            DrawSphere(8.0f, 12, 8);
            glPopMatrix();
            break;

        case ENT_CLASS_SAWBLADE: {
            const float spin = static_cast<float>(e.flags) / 100.0f;
            const float angle = std::fmod(time * (720.0f + 1440.0f * spin), 360.0f);

            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glRotatef(angle, 0.0f, 1.0f, 0.0f);
            glColor4f(0.78f, 0.8f, 0.82f, 1.0f);
            DrawGear(24.0f, 17.0f, 12);
            glPopMatrix();
            break;
        }

        case ENT_CLASS_STAPLE_BALL:
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.45f, 0.9f, 1.0f, 1.0f);
            DrawSphere(6.0f, 10, 6);
            glPopMatrix();
            break;

        case ENT_CLASS_LAST_BALL:
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.7f, 1.0f, 0.8f, 1.0f);
            DrawSphere(36.0f, 20, 14);
            glPopMatrix();
            break;

        default:
            break;
        }
    }

    // Saydam ve parlayan etkiler (derinlige yazmaz).
    void RenderTranslucent(const NetEntityState& e, float time) {
        const float age = static_cast<float>(e.health) / 100.0f;

        switch (e.classId) {
        case ENT_CLASS_STAPLE_BALL:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.3f, 0.8f, 1.0f, 0.35f);
            DrawSphere(13.0f, 10, 6);
            glPopMatrix();
            break;

        case ENT_CLASS_LAST_BALL:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.4f, 1.0f, 0.7f, 0.30f);
            DrawSphere(56.0f, 20, 14);
            glColor4f(0.5f, 1.0f, 0.8f, 0.22f);
            DrawSphere(78.0f * (0.92f + 0.08f * std::sin(time * 14.0f)), 20, 14);
            glPopMatrix();
            break;

        case ENT_CLASS_LAVA_BOMB:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(1.0f, 0.45f, 0.1f, 0.35f);
            DrawSphere(15.0f, 10, 6);
            glPopMatrix();
            break;

        case ENT_CLASS_EXPLOSION: {
            const float t = ClampF(age / EXPLOSION_EFFECT_LIFE, 0.0f, 1.0f);
            const float radius = static_cast<float>(e.flags) * (0.25f + 0.75f * std::sqrt(t));
            const float alpha = 1.0f - t;

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(1.0f, 0.65f - 0.3f * t, 0.2f, 0.55f * alpha);
            DrawSphere(radius, 20, 14);
            glColor4f(1.0f, 0.95f, 0.7f, 0.8f * alpha);
            DrawSphere(radius * 0.55f, 16, 10);
            glPopMatrix();
            break;
        }

        case ENT_CLASS_BEAM: {
            const float t = ClampF(age / BEAM_EFFECT_LIFE, 0.0f, 1.0f);
            const float alpha = 1.0f - t;
            const float length = static_cast<float>(e.flags);
            const float width = 5.0f * (1.0f - 0.5f * t);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            ApplyDirection(e.yaw, e.pitch);

            // Iki capraz seritle her acidan gorunur; icte daha ince ve parlak cekirdek.
            for (int pass = 0; pass < 2; pass++) {
                const float w = (pass == 0) ? width : width * 0.35f;
                if (pass == 0) glColor4f(0.3f, 0.9f, 1.0f, 0.55f * alpha);
                else glColor4f(0.9f, 1.0f, 1.0f, 0.95f * alpha);

                glBegin(GL_QUADS);
                glVertex3f(0.0f, -w, 0.0f); glVertex3f(length, -w, 0.0f);
                glVertex3f(length, w, 0.0f); glVertex3f(0.0f, w, 0.0f);
                glVertex3f(0.0f, 0.0f, -w); glVertex3f(length, 0.0f, -w);
                glVertex3f(length, 0.0f, w); glVertex3f(0.0f, 0.0f, w);
                glEnd();
            }
            glPopMatrix();
            break;
        }

        case ENT_CLASS_FOG_CLOUD: {
            // Omrun basinda ve sonunda yumusakca belirir/kaybolur.
            const float life = e.pitch;
            const float fade = ClampF(MinF2(age / 0.6f, (life - age) / 2.0f), 0.0f, 1.0f);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y, e.origin.z);
            glColor4f(0.72f, 0.74f, 0.78f, 0.38f * fade);
            DrawSphere(static_cast<float>(e.flags), 24, 16); // iki yuzlu: icerden de gorunur
            glPopMatrix();
            break;
        }

        case ENT_CLASS_LAVA_POOL: {
            const float life = e.pitch;
            const float fade = ClampF(MinF2(age / 0.4f, (life - age) / 1.5f), 0.0f, 1.0f);
            const float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPushMatrix();
            glTranslatef(e.origin.x, e.origin.y + 1.0f, e.origin.z);
            glColor4f(1.0f, 0.3f + 0.2f * pulse, 0.05f, 0.85f * fade);
            DrawDisc(static_cast<float>(e.flags), 28);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glColor4f(1.0f, 0.5f, 0.1f, 0.25f * fade);
            glTranslatef(0.0f, 1.0f, 0.0f);
            DrawDisc(static_cast<float>(e.flags) * 1.08f, 28);
            glPopMatrix();
            break;
        }

        default:
            break;
        }
    }
}

void WeaponEffects_Render(const std::vector<NetEntityState>& entities) {
    bool any = false;
    for (const NetEntityState& e : entities) {
        if (e.classId >= ENT_CLASS_ROCKET) { any = true; break; }
    }
    if (!any) return;

    const float time = static_cast<float>(SDL_GetTicks()) * 0.001f;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    for (const NetEntityState& e : entities) RenderSolid(e, time);

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    for (const NetEntityState& e : entities) RenderTranslucent(e, time);

    glPopAttrib();
}