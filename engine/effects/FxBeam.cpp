#include "FxEmitters.h"
#include "FxTuning.h"
#include <algorithm>
#include <cmath>
#include <windows.h>
#include <GL/gl.h>

namespace {

    namespace Tune = FxTuning::RayBeam;

    //==========================================================================
    // Yardimcilar
    //==========================================================================

    // Isin boyunca kameraya donuk bir seridin yan vektoru (yari genislik uzunlugunda).
    glm::vec3 RibbonSide(const glm::vec3& start, const glm::vec3& end, float halfWidth, const Fx::CameraBasis& camera) {
        const glm::vec3 axis = glm::normalize(end - start);
        const glm::vec3 middle = (start + end) * 0.5f;

        // Eksen ve kameraya bakis yonunun capraz carpimi seridi kameraya dondurur.
        const glm::vec3 side = glm::cross(axis, glm::normalize(camera.position - middle));

        // Isina tam onden bakilirsa capraz carpim sifirlanir; o zaman kameranin sag vektoru kullanilir.
        if (glm::length(side) > 0.001f) return glm::normalize(side) * halfWidth;
        return camera.right * halfWidth;
    }

    // Dokulu serit (beam.tga kullanir).
    void DrawTexturedStrip(const glm::vec3& start, const glm::vec3& end, float halfWidth, const glm::vec4& color,
        const Fx::CameraBasis& camera, GLuint texture) {

        const glm::vec3 side = RibbonSide(start, end, halfWidth, camera);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glColor4f(color.r, color.g, color.b, color.a);

        glBegin(GL_QUADS);
        Fx::EmitTexturedVertex(0.0f, 0.0f, start - side);
        Fx::EmitTexturedVertex(1.0f, 0.0f, start + side);
        Fx::EmitTexturedVertex(1.0f, 1.0f, end + side);
        Fx::EmitTexturedVertex(0.0f, 1.0f, end - side);
        glEnd();
    }

    void EmitColoredVertex(const glm::vec3& position, const glm::vec3& rgb, float alpha) {
        glColor4f(rgb.r, rgb.g, rgb.b, alpha);
        glVertex3f(position.x, position.y, position.z);
    }

    // Dokusuz yedek serit: ortasi opak, kenarlari seffaf; yumusak bir isin gibi gorunur.
    // Yalnizca beam.tga yokken ve "fx_fallback_shapes" aciksa kullanilir.
    void DrawSoftRibbon(const glm::vec3& start, const glm::vec3& end, float halfWidth, const glm::vec3& rgb, float alpha,
        const Fx::CameraBasis& camera) {

        const glm::vec3 side = RibbonSide(start, end, halfWidth, camera);

        glDisable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);

        // Sol yarisi: kenar (alfa 0) -> orta (alfa)
        EmitColoredVertex(start - side, rgb, 0.0f);
        EmitColoredVertex(start, rgb, alpha);
        EmitColoredVertex(end, rgb, alpha);
        EmitColoredVertex(end - side, rgb, 0.0f);

        // Sag yarisi: orta (alfa) -> kenar (alfa 0)
        EmitColoredVertex(start, rgb, alpha);
        EmitColoredVertex(start + side, rgb, 0.0f);
        EmitColoredVertex(end + side, rgb, 0.0f);
        EmitColoredVertex(end, rgb, alpha);

        glEnd();
    }

    // Kameraya donuk tek bir kare (glBegin(GL_QUADS) ile glEnd arasinda cagrilir).
    void EmitBillboard(const glm::vec3& center, float halfSize, const Fx::CameraBasis& camera) {
        const glm::vec3 right = camera.right * halfSize;
        const glm::vec3 up = camera.up * halfSize;

        Fx::EmitTexturedVertex(0.0f, 1.0f, center - right - up);
        Fx::EmitTexturedVertex(1.0f, 1.0f, center + right - up);
        Fx::EmitTexturedVertex(1.0f, 0.0f, center + right + up);
        Fx::EmitTexturedVertex(0.0f, 0.0f, center - right + up);
    }

    //==========================================================================
    // Isin govdesi: yumusak hale ve parlak cekirdek
    //==========================================================================
    void DrawBeamBody(const glm::vec3& start, const glm::vec3& end, float age, const Fx::CameraBasis& camera) {
        const GLuint beamTexture = SpriteSystem::Get(SPR_BEAM).texture;
        const bool textured = (beamTexture != 0);

        // Dosya yoksa ve yedek sekiller kapaliysa govde cizilmez.
        if (!textured && !Fx::FallbackShapesEnabled()) return;

        // Cekirdek hizli, hale daha yavas soner.
        const float coreFade = Fx::Clamp(1.0f - age / Tune::kCoreLife, 0.0f, 1.0f);
        const float glowFade = Fx::Clamp(1.0f - age / Tune::kGlowLife, 0.0f, 1.0f);

        if (glowFade > 0.0f) {
            // Hale yaslandikca genisler.
            const float halfWidth = Tune::kGlowBaseWidth * (1.0f + age * Tune::kGlowWidening);
            const float alpha = Tune::kGlowAlpha * glowFade;

            if (textured) DrawTexturedStrip(start, end, halfWidth, glm::vec4(Tune::kGlowColor, alpha), camera, beamTexture);
            else DrawSoftRibbon(start, end, halfWidth, Tune::kGlowColor, alpha, camera);
        }

        if (coreFade > 0.0f) {
            // Cekirdek sonerken biraz daralir (kCoreMinWidthFraction'a kadar).
            const float widthFactor = Tune::kCoreMinWidthFraction + (1.0f - Tune::kCoreMinWidthFraction) * coreFade;
            const float halfWidth = Tune::kCoreBaseWidth * widthFactor;

            if (textured) DrawTexturedStrip(start, end, halfWidth, glm::vec4(Tune::kCoreColor, coreFade), camera, beamTexture);
            else DrawSoftRibbon(start, end, halfWidth, Tune::kCoreColor, coreFade, camera);
        }

        glDisable(GL_TEXTURE_2D);
    }

    //==========================================================================
    // Sarmal: isini saran iki sira parlayan nokta
    //==========================================================================

    // Sarmalin her noktasi icin visitor(konum, renk, alfa, yari boyut) cagirir. Hem dokulu hem de
    // dokusuz cizim ayni konumlari kullanir.
    template <typename Visitor>
    void ForEachSpiralDot(const glm::vec3& start, const glm::vec3& direction, float length, float age, Visitor visit) {
        // Isina dik iki eksen, sarmalin dairesini tanimlar.
        const glm::vec3 helper = (std::fabs(direction.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 circleU = glm::normalize(glm::cross(direction, helper));
        const glm::vec3 circleV = glm::cross(direction, circleU);

        const float fade = Fx::Clamp(1.0f - age / BEAM_EFFECT_LIFE, 0.0f, 1.0f);
        // Sarmal zamanla isindan uzaklasip genisler.
        const float spread = Fx::Smoothstep(0.0f, BEAM_EFFECT_LIFE * Tune::kSpiralSpreadEnd, age);
        const float spiralRadius = Tune::kSpiralStartRadius + Tune::kSpiralExtraRadius * spread;
        const float dotHalfSize = Tune::kSpiralDotSize * (0.6f + 0.4f * fade);   // sonerken noktalar kuculur

        // Uzun isinlarda nokta araligi acilir: nokta sayisi kSpiralMaxDots'u gecmez.
        const float spacing = std::max<>(Tune::kSpiralMinDotSpacing, length / Tune::kSpiralMaxDots);
        const int dotCount = static_cast<int>(length / spacing);

        for (int dot = 0; dot < dotCount; ++dot) {
            const float distance = static_cast<float>(dot) * spacing;
            // Aci hem isin boyunca ilerler hem de zamanla doner: sarmal "akar".
            const float phase = distance * Tune::kSpiralTurnsPerUnit - age * Tune::kSpiralSpinPerSecond;

            for (int strand = 0; strand < 2; ++strand) {
                // Iki sarmal birbirinin tam karsisinda durur (yarim tur = pi).
                const float angle = phase + static_cast<float>(strand) * Fx::kPi;
                const glm::vec3 offset = (circleU * std::cos(angle) + circleV * std::sin(angle)) * spiralRadius;

                // Parildama: konum ve zamana bagli, 0.1 ile 1.0 arasi degisen parlaklik.
                const float twinkle = 0.55f + 0.45f * std::sin(distance * Tune::kSpiralTwinkleFrequency
                    + age * Tune::kSpiralTwinkleSpeed + static_cast<float>(strand) * 2.0f);

                const glm::vec3 color = (strand == 0) ? Tune::kSpiralColorA : Tune::kSpiralColorB;
                visit(start + direction * distance + offset, color, fade * twinkle, dotHalfSize);
            }
        }
    }

    void DrawBeamSpiral(const glm::vec3& start, const glm::vec3& direction, float length, float age,
        const Fx::CameraBasis& camera) {

        const GLuint dotTexture = SpriteSystem::Get(SPR_GLOW).texture;

        if (dotTexture != 0) {
            // Sprite var: her nokta dokulu, kameraya donuk bir kare.
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, dotTexture);
            glBegin(GL_QUADS);
            ForEachSpiralDot(start, direction, length, age,
                [&](const glm::vec3& position, const glm::vec3& color, float alpha, float halfSize) {
                    glColor4f(color.r, color.g, color.b, alpha);
                    EmitBillboard(position, halfSize, camera);
                });
            glEnd();
            glDisable(GL_TEXTURE_2D);
            return;
        }

        if (!Fx::FallbackShapesEnabled()) return;

        // Sprite yok: yuvarlatilmis (smooth) piksel noktalari cizilir. Boyutlari mesafeyle
        // degismez, sabit piksel capindadir.
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_POINT_SMOOTH);
        glPointSize(Tune::kSpiralPointPixels);
        glBegin(GL_POINTS);
        ForEachSpiralDot(start, direction, length, age,
            [](const glm::vec3& position, const glm::vec3& color, float alpha, float) {
                glColor4f(color.r, color.g, color.b, alpha);
                glVertex3f(position.x, position.y, position.z);
            });
        glEnd();
        glDisable(GL_POINT_SMOOTH);
    }

}  // namespace

void Fx::SpawnRayBeamFlashes(const NetEntityState& beam) {
    const glm::vec3 direction = DirectionFromAngles(beam.yaw, beam.pitch);
    const glm::vec3 muzzle = beam.origin;
    const glm::vec3 impact = beam.origin + direction * RadiusOf(beam);   // isin uzunlugu flags alaninda tasinir

    ParticleBuilder(SPR_GLOW)
        .At(muzzle)
        .Life(Tune::kMuzzleFlashLife)
        .Size(Tune::kMuzzleFlashStartSize, Tune::kMuzzleFlashEndSize)
        .Color(Tune::kMuzzleFlashColor)
        .Additive()
        .Spawn();

    ParticleBuilder(SPR_GLOW)
        .At(impact)
        .Life(Tune::kImpactFlashLife)
        .Size(Tune::kImpactFlashStartSize, Tune::kImpactFlashEndSize)
        .Color(Tune::kImpactFlashColor)
        .Additive()
        .Spawn();

    ParticleBuilder(SPR_RING)
        .At(impact)
        .Life(Tune::kImpactRingLife)
        .Size(Tune::kImpactRingStartSize, Tune::kImpactRingEndSize)
        .Color(Tune::kImpactRingColor)
        .Additive()
        .Spawn();

    for (int i = 0; i < Tune::kImpactSparkCount; ++i) {
        ParticleBuilder(SPR_SPARK)
            .At(impact)
            .Velocity(RandomDirection() * Tune::kImpactSparkSpeed.Random())
            .Life(Tune::kImpactSparkLife.Random())
            .Size(Tune::kImpactSparkStartSize, Tune::kImpactSparkEndSize)
            .Color(Tune::kImpactSparkColor)
            .Gravity(Tune::kImpactSparkGravity)
            .Drag(Tune::kImpactSparkDrag)
            .Additive()
            .Spawn();
    }
}

void Fx::DrawRayBeam(const NetEntityState& beam, const CameraBasis& camera) {
    const float length = RadiusOf(beam);
    if (length < 1.0f) return;

    const float age = AgeSeconds(beam);
    const glm::vec3 direction = DirectionFromAngles(beam.yaw, beam.pitch);
    const glm::vec3 start = beam.origin;
    const glm::vec3 end = start + direction * length;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // parlayan, ust uste bindikce parlaklasan karisim

    DrawBeamBody(start, end, age, camera);
    DrawBeamSpiral(start, direction, length, age, camera);
}