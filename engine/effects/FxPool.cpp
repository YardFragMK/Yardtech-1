#include "FxEmitters.h"
#include "FxTuning.h"
#include "../renderer/ShaderLibrary.h"
#include "../renderer/ShaderProgram.h"
#include "../GLExtensions.h"
#include "../console/Console.h"
#include <cmath>
#include <string>

namespace {

    namespace Tune = FxTuning::Pool;

    // EmitterState::timers dizisindeki yuvalarin anlami.
    enum TimerSlot { kBubbleTimer = 0, kSteamTimer = 1, kEmberTimer = 2 };

    const Tune::Style& StyleFor(bool isAcid) {
        return isAcid ? Tune::kAcid : Tune::kLava;
    }

    // Havuz carptigi yuzeye yatar; o yuzeyin egimi bir kez aranir.
    void EnsureSurfaceNormal(const NetEntityState& pool, Fx::EmitterState& state) {
        if (state.surfaceNormalKnown) return;

        state.surfaceNormal = Fx::GroundNormalAt(pool.origin);
        state.surfaceNormalKnown = true;
    }

    // Havuzun yuzey duzlemi: parcaciklari yuzeye dagitmak icin kullanilir.
    struct PoolSurface {
        glm::vec3 origin;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        float radius;

        // Havuzun uzerinde, hafif yukarida rastgele bir nokta. spread = yaricapin kesri.
        glm::vec3 RandomPoint(float spread) const {
            const float angle = Fx::RandomFloat(0.0f, 2.0f * Fx::kPi);
            // Karekok: noktalar daire alanina esit dagilsin (merkezde yigilmasin).
            const float distance = radius * spread * std::sqrt(Fx::RandomFloat(0.0f, 1.0f));
            const glm::vec3 offset = (tangent * std::cos(angle) + bitangent * std::sin(angle)) * distance;

            return origin + offset + normal * Tune::kSurfaceLift;
        }
    };

    PoolSurface MakeSurface(const NetEntityState& pool, const Fx::EmitterState& state) {
        PoolSurface surface;
        surface.origin = pool.origin;
        surface.normal = state.surfaceNormal;
        surface.radius = Fx::RadiusOf(pool);
        Fx::BuildPlaneBasis(surface.normal, surface.tangent, surface.bitangent);
        return surface;
    }

    //==========================================================================
    // PARCACIKLAR
    //==========================================================================

    // Carpma ani: parlayan damlalar sicrar ve yere geri duser.
    void SpawnImpactSplash(const PoolSurface& surface, const Tune::Style& style) {
        const Fx::ColorRamp dropletColor = {
            glm::vec4(style.hotColor, 1.0f),
            glm::vec4(style.hotColor * 0.5f, 0.0f)   // sonda yarisi kadar parlak ve saydam
        };

        for (int i = 0; i < Tune::kDropletCount; ++i) {
            // Cogunlukla yuzeyden uzaga, biraz da rastgele yanlara.
            const glm::vec3 upward = surface.normal * Tune::kDropletNormalWeight.Random();
            const glm::vec3 scatter = Fx::RandomDirection() * Tune::kDropletRandomWeight;
            const glm::vec3 direction = glm::normalize(upward + scatter);

            Fx::ParticleBuilder(SPR_GLOW)
                .At(surface.origin + surface.normal * 4.0f)   // 4 birim yukarida baslar
                .Velocity(direction * Tune::kDropletSpeed.Random())
                .Life(Tune::kDropletLife.Random())
                .Size(Tune::kDropletStartSize, Tune::kDropletEndSize)
                .Color(dropletColor)
                .Gravity(Tune::kDropletGravity)
                .Additive()
                .Spawn();
        }
    }

    void SpawnImpactSmoke(const PoolSurface& surface, const Tune::Style& style) {
        const Fx::ColorRamp smokeColor = {
            glm::vec4(style.darkColor, Tune::kImpactSmokeAlpha),
            glm::vec4(style.darkColor * 0.5f, 0.0f)
        };

        for (int i = 0; i < Tune::kImpactSmokeCount; ++i) {
            Fx::ParticleBuilder(SPR_SMOKE)
                .At(surface.RandomPoint(Tune::kImpactSmokeSpread))
                .Velocity(surface.normal * Tune::kImpactSmokeRise.Random())
                .Life(Tune::kImpactSmokeLife.Random())
                .Size(surface.radius * Tune::kImpactSmokeStartScale, surface.radius * Tune::kImpactSmokeEndScale)
                .Color(smokeColor)
                .Drag(Tune::kImpactSmokeDrag)
                .RandomRotation()
                .FadeIn(Tune::kImpactSmokeFadeIn)
                .FadeNearCamera(Tune::kImpactSmokeNearFade)
                .Spawn();
        }
    }

    // Kabarciklar yavasca yukselir, sisip patlar.
    void EmitBubbles(const PoolSurface& surface, const Tune::Style& style, Fx::EmitterState& state, float deltaTime) {
        // Buyuk havuz daha sik kabarcik cikarir: aralik = olcek / yaricap.
        const float interval = style.bubbleIntervalScale / surface.radius;
        const int dueCount = Fx::CountDueTicks(state.timers[kBubbleTimer], interval, deltaTime);

        const Fx::ColorRamp bubbleColor = {
            glm::vec4(style.hotColor, Tune::kBubbleAlpha),
            glm::vec4(style.hotColor, 0.0f)
        };

        for (int i = 0; i < dueCount; ++i) {
            Fx::ParticleBuilder(SPR_BUBBLE)
                .At(surface.RandomPoint(Tune::kBubbleSpread))
                .Velocity(surface.normal * Tune::kBubbleRise.Random())
                .Life(Tune::kBubbleLife.Random())
                .Size(Tune::kBubbleStartSize, Tune::kBubbleEndSize.Random())
                .Color(bubbleColor)
                .Additive()
                .Spawn();
        }
    }

    void EmitSteam(const PoolSurface& surface, const Tune::Style& style, Fx::EmitterState& state, float deltaTime) {
        const float interval = style.steamIntervalScale / surface.radius;
        const int dueCount = Fx::CountDueTicks(state.timers[kSteamTimer], interval, deltaTime);

        for (int i = 0; i < dueCount; ++i) {
            const glm::vec3 rise = surface.normal * Tune::kSteamRise.Random();
            const glm::vec3 sideways = Fx::RandomDirection() * Tune::kSteamSideways;

            Fx::ParticleBuilder(SPR_SMOKE)
                .At(surface.RandomPoint(Tune::kSteamSpread))
                .Velocity(rise + sideways)
                .Life(Tune::kSteamLife.Random())
                .Size(Tune::kSteamStartSize, Tune::kSteamEndSize)
                .Color(style.steamColor)
                .RandomRotation(Tune::kSteamSpin)
                .FadeIn(Tune::kSteamFadeIn)
                .FadeNearCamera(Tune::kSteamNearFade)
                .Spawn();
        }
    }

    // Lavda yukselen koz, asitte yere geri duşen fisirtilar.
    void EmitEmbers(const PoolSurface& surface, const Tune::Style& style, Fx::EmitterState& state, float deltaTime) {
        const float interval = style.emberIntervalScale / surface.radius;
        const int dueCount = Fx::CountDueTicks(state.timers[kEmberTimer], interval, deltaTime);

        const Fx::ColorRamp emberColor = {
            glm::vec4(style.hotColor, 1.0f),
            glm::vec4(style.hotColor * 0.4f, 0.0f)
        };

        for (int i = 0; i < dueCount; ++i) {
            const glm::vec3 launch = surface.normal * style.emberSpeed.Random();
            const glm::vec3 scatter = Fx::RandomDirection() * Tune::kEmberSideways;

            Fx::ParticleBuilder(SPR_SPARK)
                .At(surface.RandomPoint(Tune::kEmberSpread))
                .Velocity(launch + scatter)
                .Acceleration(surface.normal * style.emberLift)
                .Life(style.emberLife.Random())
                .Size(style.emberSize, Tune::kEmberEndSize)
                .Color(emberColor)
                .Additive()
                .Spawn();
        }
    }

    //==========================================================================
    // HAVUZ YUZEYININ CIZIMI
    //==========================================================================

    // Bu karede bir havuzu cizmek icin gereken her sey.
    struct PoolDrawInfo {
        glm::vec3 center;       // yuzey uzerindeki merkez (zemine gomulmesin diye hafif yukarida)
        glm::vec3 tangent;      // yuzeyin iki yatay ekseni
        glm::vec3 bitangent;
        float radius;           // su anki yaricap (havuz acilirken buyur)
        float fade;             // genel opaklik 0..1 (omrun sonunda azalir)
        float timeSeconds;
        float seed;             // havuz basina 0..1 sayi: kenar sekli her havuzda farkli olur
    };

    // Havuzun bu karede gorunup gorunmedigini belirler ve cizim bilgisini doldurur.
    bool BuildDrawInfo(const NetEntityState& pool, Fx::EmitterState& state, float timeSeconds, PoolDrawInfo& out) {
        EnsureSurfaceNormal(pool, state);

        const float age = Fx::AgeSeconds(pool);
        const float spreadProgress = Fx::Clamp(age / Tune::kGrowTime, 0.0f, 1.0f);
        // Ucuncu kuvvet ile "ease-out": havuz hizli acilir, sonda yavaslar.
        const float growth = 1.0f - std::pow(1.0f - spreadProgress, 3.0f);
        const float fade = Fx::Clamp((pool.pitch - age) / Tune::kFadeOutTime, 0.0f, 1.0f);
        if (growth <= Tune::kMinVisibleGrowth || fade <= 0.0f) return false;

        out.center = pool.origin + state.surfaceNormal * Tune::kDrawLift;
        Fx::BuildPlaneBasis(state.surfaceNormal, out.tangent, out.bitangent);
        out.radius = Fx::RadiusOf(pool) * growth;
        out.fade = fade;
        out.timeSeconds = timeSeconds;
        out.seed = static_cast<float>(pool.id % 97) / 97.0f;
        return true;
    }

    // Bir kez uyari verir: havuz neden cizilmiyor?
    void ReportMissingLiquidTexture(bool isAcid) {
        static bool reportedLava = false;
        static bool reportedAcid = false;

        bool& reported = isAcid ? reportedAcid : reportedLava;
        if (reported) return;
        reported = true;

        Console::Log(std::string("Havuz cizilmedi: sprites/") + (isAcid ? "acid.tga" : "lava.tga")
            + " yuklenemedi ('fx_status' ile dosya durumunu gorun)");
    }

    //--------------------------------------------------------------------------
    // Yol 1: poolmask.tga ve "pool" shader'i varsa. Kenar sekli maskeden, akis shader'dan gelir.
    //--------------------------------------------------------------------------
    void DrawWithShader(const PoolDrawInfo& info, bool isAcid, GLuint liquidTexture, GLuint maskTexture,
        ShaderProgram& shader) {

        // Maske dokusundaki sekil karenin tamamini kaplamaz; kareyi ona gore buyuk ciziyoruz.
        const float halfSize = info.radius / Tune::kMaskAverageRadius;
        const glm::vec3 across = info.tangent * halfSize;
        const glm::vec3 along = info.bitangent * halfSize;

        glActiveTexture_(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, liquidTexture);
        glActiveTexture_(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, maskTexture);
        glActiveTexture_(GL_TEXTURE0);

        shader.Bind();
        shader.SetInt("uLiquid", 0);
        shader.SetInt("uMask", 1);
        shader.SetFloat("uTime", info.timeSeconds);
        shader.SetFloat("uAlpha", info.fade);
        shader.SetInt("uMode", isAcid ? 1 : 0);

        glBegin(GL_QUADS);
        Fx::EmitTexturedVertex(0.0f, 0.0f, info.center - across - along);
        Fx::EmitTexturedVertex(1.0f, 0.0f, info.center + across - along);
        Fx::EmitTexturedVertex(1.0f, 1.0f, info.center + across + along);
        Fx::EmitTexturedVertex(0.0f, 1.0f, info.center - across + along);
        glEnd();

        ShaderProgram::Unbind();

        glActiveTexture_(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture_(GL_TEXTURE0);
    }

    //--------------------------------------------------------------------------
    // Yol 2: yalnizca lava.tga / acid.tga varsa. Kenar sekli kodla hesaplanir, akis doku
    // koordinatlarinin kaydirilmasiyla saglanir.
    //--------------------------------------------------------------------------

    // Havuzun kenar yaricapi (1.0 = ortalama), kenar acisina gore dalgalanir. Uc farkli dalga
    // toplanir; seed her havuza farkli bir sekil verir.
    float RimRadius(float angle, float seed) {
        const float phase = seed * 2.0f * Fx::kPi;

        return 1.0f
            + Tune::kRimWobbleLow * std::sin(3.0f * angle + phase)           // 3 loblu buyuk dalga
            + Tune::kRimWobbleMid * std::sin(7.0f * angle + phase * 2.3f)    // 7 loblu orta dalga
            + Tune::kRimWobbleHigh * std::sin(13.0f * angle + phase * 3.7f); // 13 loblu ince dalga
    }

    // Havuzun tek bir katmani. Iki katman farkli olcek ve yonde kayarak akis hissi verir.
    void DrawPoolLayer(const PoolDrawInfo& info, const Tune::Style& style, float tileScale, float flowDirection,
        float layerAlpha) {

        // Nabiz: merkezin parlakligi yavasca artip azalir.
        const float pulse = 1.0f + style.pulseAmount * std::sin(info.timeSeconds * Tune::kPulseSpeed);

        // Dunya birimini doku koordinatina ceviren olcek ve zamanla kayma.
        const float repeatsPerUnit = tileScale / Tune::kLiquidTileSize;
        const float scrollU = info.timeSeconds * style.flowSpeed * flowDirection;
        const float scrollV = info.timeSeconds * style.flowSpeed * 0.6f * flowDirection;   // v yonu daha yavas

        // Bir kose noktasi: konum, renk ve doku koordinati.
        auto emitVertex = [&](const glm::vec3& offset, const glm::vec3& tint, float alpha) {
            glColor4f(tint.r * pulse, tint.g * pulse, tint.b * pulse, alpha * layerAlpha * info.fade);

            const float u = glm::dot(offset, info.tangent) * repeatsPerUnit + scrollU;
            const float v = glm::dot(offset, info.bitangent) * repeatsPerUnit + scrollV;
            glTexCoord2f(u, v);

            const glm::vec3 position = info.center + offset;
            glVertex3f(position.x, position.y, position.z);
            };

        glBegin(GL_TRIANGLE_FAN);
        emitVertex(glm::vec3(0.0f), style.centerTint, 1.0f);   // yelpazenin merkezi

        for (int i = 0; i <= Tune::kRimSegments; ++i) {       // <= : son nokta ilkiyle birlesir
            const float angle = 2.0f * Fx::kPi * static_cast<float>(i) / static_cast<float>(Tune::kRimSegments);
            const float rim = RimRadius(angle, info.seed) * info.radius;
            const glm::vec3 offset = (info.tangent * std::cos(angle) + info.bitangent * std::sin(angle)) * rim;

            emitVertex(offset, style.edgeTint, Tune::kEdgeAlpha);
        }
        glEnd();
    }

    void DrawWithoutShader(const PoolDrawInfo& info, bool isAcid, GLuint liquidTexture) {
        const Tune::Style& style = StyleFor(isAcid);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, liquidTexture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);   // doku rengi x kose rengi

        // Alt katman: dokunun kendisi, bir yone akar.
        DrawPoolLayer(info, style, 1.0f, 1.0f, 1.0f);
        // Ust katman: daha sik tekrar, ters yone akar; iki katman birlikte derinlik hissi verir.
        DrawPoolLayer(info, style, Tune::kSecondLayerScale, -1.0f, Tune::kSecondLayerAlpha);

        glDisable(GL_TEXTURE_2D);
    }

}  // namespace

void Fx::EmitPool(const NetEntityState& pool, EmitterState& state, float deltaTime, bool justAppeared, bool isAcid) {
    EnsureSurfaceNormal(pool, state);

    const Tune::Style& style = StyleFor(isAcid);
    const PoolSurface surface = MakeSurface(pool, state);
    const float age = AgeSeconds(pool);
    const float lifetime = pool.pitch;

    if (justAppeared && age < Tune::kImpactWindow) {
        SpawnImpactSplash(surface, style);
        SpawnImpactSmoke(surface, style);
    }

    // Havuz sonarken (son kisim) yeni parcacik uretilmez.
    if (age >= lifetime - Tune::kEmissionStopBeforeEnd) return;

    EmitBubbles(surface, style, state, deltaTime);
    EmitSteam(surface, style, state, deltaTime);
    EmitEmbers(surface, style, state, deltaTime);
}

void Fx::DrawLiquidPool(const NetEntityState& pool, EmitterState& state, bool isAcid, float timeSeconds) {
    // ZORUNLU: sivi dokusu. Bu dosya yoksa havuz cizilmez.
    const SpriteInfo& liquid = SpriteSystem::Get(isAcid ? SPR_ACID : SPR_LAVA);
    if (liquid.texture == 0) {
        ReportMissingLiquidTexture(isAcid);
        return;
    }

    PoolDrawInfo info;
    if (!BuildDrawInfo(pool, state, timeSeconds, info)) return;

    // ISTEGE BAGLI: kenar maskesi ve shader. Ikisi de varsa daha gelismis yol kullanilir.
    ShaderProgram* shader = ShaderLibrary::Get("pool");
    const SpriteInfo& mask = SpriteSystem::Get(SPR_POOL_MASK);
    const bool shaderPathAvailable = (shader != nullptr && mask.texture != 0 && glActiveTexture_ != nullptr);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(Tune::kPolygonOffset, Tune::kPolygonOffset);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (shaderPathAvailable) DrawWithShader(info, isAcid, liquid.texture, mask.texture, *shader);
    else DrawWithoutShader(info, isAcid, liquid.texture);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_POLYGON_OFFSET_FILL);
}