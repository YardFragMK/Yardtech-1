#version 120

// Sivi yuzeyi (su, asit, lav, kan): iki katman doku farkli yonlere kayar ve sinus dalgasiyla
// oynar. BSP doku koordinati "doku tekrari" biriminde oldugu icin sin(uv * 2pi) tekrar basina
// bir dalgadir. Parametreler sivi turune gore CPU'dan gelir.
uniform sampler2D uTexture;
uniform float uTime;
uniform float uAlpha;   // 1 = opak, daha azi saydam
uniform float uSpeed;   // dalga ve kayma hizi carpani
uniform float uWarp;    // dalga genligi (doku koordinati birimi)
uniform vec3  uTint;    // renk carpani
uniform float uGlow;    // nabiz atan ek parlaklik (lav, asit)

void main() {
    vec2 uv = gl_TexCoord[0].st;
    float t = uTime * uSpeed;

    vec2 warp = vec2(
        sin(uv.t * 6.2832 + t * 1.4),
        sin(uv.s * 6.2832 + t * 1.1)
    ) * uWarp;

    vec3 a = texture2D(uTexture, uv + warp + vec2(t * 0.025, t * 0.018)).rgb;
    vec3 b = texture2D(uTexture, uv * 1.35 - warp + vec2(-t * 0.02, t * 0.03)).rgb;

    vec3 col = mix(a, b, 0.4) * uTint;
    col *= 1.0 + uGlow * (0.5 + 0.5 * sin(uTime * 1.7 + uv.s * 3.0 + uv.t * 2.0));

    gl_FragColor = vec4(col, uAlpha);
}