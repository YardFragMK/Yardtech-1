#version 120

// CRT monitor: ekran egriligi, renk sapmasi, fosfor parlamasi, tarama cizgileri,
// RGB maskesi, vinyet, titreme ve gren.
uniform sampler2D uScene;
uniform vec2  uResolution;
uniform float uTime;
uniform float uIntensity;   // 0..1
uniform float uScanlines;   // dikey tarama cizgisi sayisi

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = gl_TexCoord[0].st;
    vec3 original = texture2D(uScene, uv).rgb;

    // Ekran egriligi: kenarlar merkeze dogru "buzulur", disarida kalan alan monitor cercevesi (siyah).
    vec2 cc = uv - 0.5;
    float dist = dot(cc, cc);
    vec2 curved = uv + cc * dist * 0.35 * uIntensity;

    if (curved.x < 0.0 || curved.x > 1.0 || curved.y < 0.0 || curved.y > 1.0) {
        gl_FragColor = vec4(mix(original, vec3(0.0), uIntensity), 1.0);
        return;
    }

    // Renk sapmasi: kirmizi ve mavi kanallar merkezden disa dogru hafifce ayrilir.
    vec2 dir = curved - 0.5;
    float ca = 0.0020 * uIntensity;
    float r = texture2D(uScene, curved + dir * ca).r;
    float g = texture2D(uScene, curved).g;
    float b = texture2D(uScene, curved - dir * ca).b;
    vec3 col = vec3(r, g, b);

    // Fosfor parlamasi: komsu piksellerden hafif bulaniklik ve parlak alanlarda kucuk bloom.
    vec2 px = 1.0 / uResolution;
    vec3 glow = texture2D(uScene, curved + vec2(px.x * 1.5, 0.0)).rgb
              + texture2D(uScene, curved - vec2(px.x * 1.5, 0.0)).rgb
              + texture2D(uScene, curved + vec2(0.0, px.y * 1.5)).rgb
              + texture2D(uScene, curved - vec2(0.0, px.y * 1.5)).rgb;
    glow *= 0.25;
    col = mix(col, glow, 0.35 * uIntensity);
    col += max(glow - 0.6, 0.0) * 0.4 * uIntensity;

    // Tarama cizgileri
    float scan = 0.5 + 0.5 * sin(curved.y * uScanlines * 6.28318530718);
    col *= mix(1.0, 0.65 + 0.35 * scan, uIntensity);

    // RGB golge maskesi (her 3 pikselde bir kanal one cikar)
    float m = mod(gl_FragCoord.x, 3.0);
    vec3 mask = (m < 1.0) ? vec3(1.0, 0.75, 0.75)
              : (m < 2.0) ? vec3(0.75, 1.0, 0.75)
                          : vec3(0.75, 0.75, 1.0);
    col *= mix(vec3(1.0), mask, 0.5 * uIntensity);

    // Vinyet, hafif parlaklik titremesi ve gren
    col *= clamp(1.0 - dot(cc, cc) * 1.8 * uIntensity, 0.0, 1.0);
    col *= 1.0 + 0.015 * sin(uTime * 55.0) * uIntensity;
    col += (hash(curved * uResolution + uTime) - 0.5) * 0.04 * uIntensity;

    // Tarama cizgisi ve maske karartmasini telafi eder.
    col *= 1.0 + 0.2 * uIntensity;

    gl_FragColor = vec4(col, 1.0);
}