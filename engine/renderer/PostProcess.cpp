#include "PostProcess.h"
#include "ShaderLibrary.h"
#include "ShaderProgram.h"
#include "../GLExtensions.h"
#include "../ConVarRegistry.h"
#include "../Level.h"
#include "../console/Console.h"
#include "BSPMap.h"
#include "EntityParser.h"
#include <SDL.h>
#include <algorithm>
#include <cstdlib>

namespace {
    // windows.h'in min/max makrolariyla cakismamak icin yerel yardimcilar.
    float MinF(float a, float b) { return a < b ? a : b; }
    float MaxF(float a, float b) { return a > b ? a : b; }
    float ClampF(float v, float lo, float hi) { return MaxF(lo, MinF(hi, v)); }

    constexpr int MAX_SHADER_HOLES = 4;      // fragment shader'daki uHoles dizisinin boyutu
    constexpr size_t MAX_MAP_HOLES = 16;
    constexpr float CRT_SCANLINES = 300.0f;

    float ReadFloatKey(const Entity& entity, const char* key, float fallback) {
        const std::string* value = entity.Get(key);
        if (value == nullptr || value->empty()) return fallback;
        return static_cast<float>(std::atof(value->c_str()));
    }
}

bool PostProcess::Init(int width, int height) {
    m_width = width;
    m_height = height;
    m_ready = false;

    if (!GLShadersAvailable()) {
        Console::Log("GLSL desteklenmiyor: son-islem efektleri devre disi.");
        return false;
    }

    m_masterCvar = ConVarRegistry::RegisterInt("r_postfx", 1, "Son-islem efektleri ana anahtari (0/1)", CVAR_NONE);
    m_crtCvar = ConVarRegistry::RegisterFloat("r_crt", 0.0f, "CRT monitor efekti siddeti (0..1)", CVAR_NONE);
    m_dreamCvar = ConVarRegistry::RegisterFloat("r_dream", 0.0f, "Ruya efekti siddeti (0..1)", CVAR_NONE);
    m_underwaterCvar = ConVarRegistry::RegisterFloat("r_underwater", 0.0f, "Su alti efektini elle zorlar (0..1)", CVAR_NONE);
    m_blackHoleCvar = ConVarRegistry::RegisterInt("r_blackholes", 1, "Haritadaki kara delik efektleri (0/1)", CVAR_NONE);
    m_blackHoleTestCvar = ConVarRegistry::RegisterInt("r_blackhole_test", 0, "Oyuncunun 400 birim onune test kara deligi koyar (0/1)", CVAR_NONE);

    ShaderLibrary::RegisterCommands();
    ShaderLibrary::Load("crt", "fullscreen");
    ShaderLibrary::Load("underwater", "fullscreen");
    ShaderLibrary::Load("dream", "fullscreen");
    ShaderLibrary::Load("blackhole", "fullscreen");
    ShaderLibrary::Load("water"); // dunya su yuzeyleri icin (BSPMapRenderer baglandiginda kullanilir)

    CreateTexture(m_sceneTexture);
    CreateTexture(m_historyTexture);

    m_ready = true;
    return true;
}

void PostProcess::CreateTexture(GLuint& texture) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcess::CopyFramebufferTo(GLuint texture) {
    if (glActiveTexture_) glActiveTexture_(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width, m_height);
}

// Haritadaki "fx_blackhole" entity'lerini toplar. Seviye adi degistiginde yeniden taranir.
void PostProcess::RefreshBlackHoles() {
    const std::string& level = GetCurrentLevelName();
    if (level == m_holesLevel) return;

    m_holesLevel = level;
    m_blackHoles.clear();
    if (level.empty()) return;

    for (const Entity& entity : g_Map.GetEntities()) {
        if (!entity.Is("fx_blackhole")) continue;
        if (m_blackHoles.size() >= MAX_MAP_HOLES) break;

        const std::string* origin = entity.Get(EntityKeys::Origin);
        if (origin == nullptr) continue;

        BlackHole hole;
        hole.position = BSPMap::ParseOriginToEngineSpace(*origin);
        hole.radius = ClampF(ReadFloatKey(entity, "radius", 64.0f), 8.0f, 1024.0f);
        hole.strength = ClampF(ReadFloatKey(entity, "strength", 1.0f), 0.1f, 4.0f);
        m_blackHoles.push_back(hole);
    }

    if (!m_blackHoles.empty()) {
        Console::Log("Kara delik sayisi: " + std::to_string(m_blackHoles.size()));
    }
}

// Ekrana en buyuk gorunen en fazla 4 kara deligi shader'a uygun diziye yazar.
// Her giris: xy = ekran uv, z = ekran yaricapi (yukseklik biriminde), w = bukme siddeti.
int PostProcess::BuildHoleUniforms(const glm::dmat4& view, const glm::dmat4& projection,
    const ViewSetup& viewSetup, float deltaTime, float outData[16]) {

    struct Candidate { float u, v, radius, strength; };
    std::vector<Candidate> candidates;

    auto consider = [&](BlackHole& hole) {
        const glm::dvec4 clip = projection * view * glm::dvec4(hole.position, 1.0);

        // Gorus hatti engelliyse (duvar arkasi) delik yumusakca kaybolur.
        float target = 0.0f;
        if (clip.w > 1.0 && clip.w < 8000.0) {
            const TraceResult trace = g_Map.TraceLine(viewSetup.origin, hole.position, 1);
            target = (trace.fraction >= 1.0f) ? 1.0f : 0.0f;
        }
        hole.fade += (target - hole.fade) * MinF(1.0f, deltaTime * 5.0f);
        if (hole.fade < 0.01f || clip.w <= 1.0) return;

        // Perspektif izdusum: yaricap, derinlige (clip.w) bolunerek kuculur.
        const float radiusUv = static_cast<float>(0.5 * hole.radius * projection[1][1] / clip.w);

        Candidate candidate;
        candidate.u = static_cast<float>(clip.x / clip.w * 0.5 + 0.5);
        candidate.v = static_cast<float>(clip.y / clip.w * 0.5 + 0.5);
        candidate.radius = ClampF(radiusUv, 0.0f, 1.5f) * hole.fade;
        candidate.strength = hole.strength * hole.fade;
        candidates.push_back(candidate);
        };

    for (BlackHole& hole : m_blackHoles) consider(hole);
    if (m_testHoleActive) consider(m_testHole);

    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.radius > b.radius; });

    const int count = static_cast<int>(MinF(static_cast<float>(candidates.size()), static_cast<float>(MAX_SHADER_HOLES)));
    for (int i = 0; i < count; i++) {
        outData[i * 4 + 0] = candidates[i].u;
        outData[i * 4 + 1] = candidates[i].v;
        outData[i * 4 + 2] = candidates[i].radius;
        outData[i * 4 + 3] = candidates[i].strength;
    }
    return count;
}

// Mevcut ekrani sahne dokusuna kopyalar ve programi ortak uniform'larla baglar.
void PostProcess::BeginPass(ShaderProgram& program) {
    CopyFramebufferTo(m_sceneTexture);

    program.Bind();
    program.SetInt("uScene", 0);
    program.SetInt("uHistory", 1);
    program.SetVec2("uResolution", static_cast<float>(m_width), static_cast<float>(m_height));
    program.SetFloat("uTime", m_time);
}

void PostProcess::EndPass() {
    const float w = static_cast<float>(m_width);
    const float h = static_cast<float>(m_height);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(w, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(w, h);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, h);
    glEnd();

    ShaderProgram::Unbind();
}

void PostProcess::Apply(const glm::dmat4& view, const glm::dmat4& projection, const ViewSetup& viewSetup) {
    if (!m_ready) return;
    if (m_masterCvar != nullptr && *m_masterCvar == 0) {
        m_dreamWasActive = false;
        return;
    }

    const uint32_t ticks = SDL_GetTicks();
    float deltaTime = (m_lastTicks == 0) ? 0.016f : static_cast<float>(ticks - m_lastTicks) * 0.001f;
    deltaTime = ClampF(deltaTime, 0.0f, 0.1f);
    m_lastTicks = ticks;
    m_time = static_cast<float>(ticks) * 0.001f;

    // Test deligi: cvar acildigi anda oyuncunun onune sabitlenir.
    const bool wantTestHole = (m_blackHoleTestCvar != nullptr && *m_blackHoleTestCvar != 0);
    if (wantTestHole && !m_testHoleActive) {
        m_testHole.position = viewSetup.origin + ViewForward(viewSetup) * 400.0f;
        m_testHole.radius = 48.0f;
        m_testHole.strength = 1.0f;
        m_testHole.fade = 0.0f;
        m_testHoleActive = true;
    }
    else if (!wantTestHole) {
        m_testHoleActive = false;
    }

    RefreshBlackHoles();

    const float crt = (m_crtCvar != nullptr) ? ClampF(*m_crtCvar, 0.0f, 1.0f) : 0.0f;
    const float dream = (m_dreamCvar != nullptr) ? ClampF(*m_dreamCvar, 0.0f, 1.0f) : 0.0f;
    const float manualUnderwater = (m_underwaterCvar != nullptr) ? ClampF(*m_underwaterCvar, 0.0f, 1.0f) : 0.0f;

    // Oyuncunun gozu bir sivinin icinde mi? (worldspawn yaprak icerigi)
    const int liquid = g_Map.GetLiquidAt(viewSetup.origin);
    if (liquid != LIQUID_NONE) m_liquidKind = liquid;
    m_underwaterTarget = m_underwaterExternal || (liquid != LIQUID_NONE);
    m_underwaterAuto += ((m_underwaterTarget ? 1.0f : 0.0f) - m_underwaterAuto) * MinF(1.0f, deltaTime * 6.0f);
    const float underwater = MaxF(manualUnderwater, m_underwaterAuto);

    ShaderProgram* crtShader = ShaderLibrary::Get("crt");
    ShaderProgram* dreamShader = ShaderLibrary::Get("dream");
    ShaderProgram* underwaterShader = ShaderLibrary::Get("underwater");
    ShaderProgram* holeShader = ShaderLibrary::Get("blackhole");

    float holeData[MAX_SHADER_HOLES * 4] = {};
    int holeCount = 0;
    if (holeShader != nullptr && m_blackHoleCvar != nullptr && *m_blackHoleCvar != 0) {
        holeCount = BuildHoleUniforms(view, projection, viewSetup, deltaTime, holeData);
    }

    const bool doUnderwater = underwater > 0.001f && underwaterShader != nullptr;
    const bool doHoles = holeCount > 0;
    const bool doDream = dream > 0.001f && dreamShader != nullptr;
    const bool doCrt = crt > 0.001f && crtShader != nullptr;

    if (!doDream) m_dreamWasActive = false;
    if (!doUnderwater && !doHoles && !doDream && !doCrt) return;

    // Cizim durumu: sonunda glPopAttrib ile eski haline doner (program baglamasi haric,
    // o ayrica Unbind edilir).
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
        GL_VIEWPORT_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);

    glViewport(0, 0, m_width, m_height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m_width, 0, m_height, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    if (doUnderwater) {
        // Renk ve yogunluk siviya gore: su mavi-yesil, asit (slime) yesil, lav turuncu-kirmizi.
        float tint[3] = { 0.35f, 0.65f, 0.75f };
        float density = 1.0f;
        if (m_liquidKind == LIQUID_SLIME) {
            tint[0] = 0.45f; tint[1] = 0.85f; tint[2] = 0.30f;
            density = 1.4f;
        }
        else if (m_liquidKind == LIQUID_LAVA) {
            tint[0] = 1.00f; tint[1] = 0.35f; tint[2] = 0.08f;
            density = 2.2f;
        }

        BeginPass(*underwaterShader);
        underwaterShader->SetFloat("uIntensity", underwater);
        underwaterShader->SetVec3("uTint", tint[0], tint[1], tint[2]);
        underwaterShader->SetFloat("uDensity", density);
        EndPass();
    }

    if (doHoles) {
        BeginPass(*holeShader);
        holeShader->SetFloat("uAspect", static_cast<float>(m_width) / static_cast<float>(m_height));
        holeShader->SetInt("uHoleCount", holeCount);
        holeShader->SetVec4Array("uHoles", holeData, MAX_SHADER_HOLES);
        EndPass();
    }

    if (doDream) {
        // Efekt yeni acildiysa gecmis kare, eski bir goruntu degil mevcut sahne olmali.
        if (!m_dreamWasActive) {
            CopyFramebufferTo(m_historyTexture);
            m_dreamWasActive = true;
        }

        BeginPass(*dreamShader);
        if (glActiveTexture_) {
            glActiveTexture_(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_historyTexture);
            glActiveTexture_(GL_TEXTURE0);
        }
        dreamShader->SetFloat("uIntensity", dream);
        EndPass();

        // Bu karenin sonucu, bir sonraki karede "gecmis" olarak kullanilir.
        CopyFramebufferTo(m_historyTexture);
    }

    if (doCrt) {
        BeginPass(*crtShader);
        crtShader->SetFloat("uIntensity", crt);
        crtShader->SetFloat("uScanlines", CRT_SCANLINES);
        EndPass();
    }

    // Doku birimlerini temizle.
    if (glActiveTexture_) {
        glActiveTexture_(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture_(GL_TEXTURE0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}