#include "FxEmitters.h"
#include "../renderer/ShaderLibrary.h"
#include "../renderer/ShaderProgram.h"
#include <windows.h>
#include <GL/gl.h>

// A sparkling ball: bright core, soft halo and rotating star rays, all computed by the
// "orb" shader on a camera-facing square. Without the shader file nothing is drawn.
void Fx::DrawEnergyOrb(const glm::vec3& center, float size, const glm::vec3& color, float seed, float spikes,
    float timeSeconds, const CameraBasis& camera) {

    ShaderProgram* shader = ShaderLibrary::Get("orb");
    if (shader == nullptr) return;

    const float halfSize = size * 0.5f;
    const glm::vec3 right = camera.right * halfSize;
    const glm::vec3 up = camera.up * halfSize;

    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_ONE, GL_ONE);   // additive: the shader output is the brightness itself

    shader->Bind();
    shader->SetVec3("uColor", color.r, color.g, color.b);
    shader->SetFloat("uTime", timeSeconds);
    shader->SetFloat("uSeed", seed);
    shader->SetFloat("uSpikes", spikes);

    // The shader works in -1..1 square coordinates.
    glBegin(GL_QUADS);
    EmitTexturedVertex(-1.0f, -1.0f, center - right - up);
    EmitTexturedVertex(1.0f, -1.0f, center + right - up);
    EmitTexturedVertex(1.0f, 1.0f, center + right + up);
    EmitTexturedVertex(-1.0f, 1.0f, center - right + up);
    glEnd();

    ShaderProgram::Unbind();
}