#version 120

// Su alti: sinusoidal goruntu dalgalanmasi, mavi-yesil renk, bulaniklik, kenar karartmasi ve
// isik (caustic) titresimleri.
uniform sampler2D uScene;
uniform vec2  uResolution;
uniform float uTime;
uniform float uIntensity;   // 0..1

void main() {
    vec2 uv = gl_TexCoord[0].st;
    float t = uTime;

    vec2 wobble = vec2(
        sin(uv.y * 18.0 + t * 1.7) + sin(uv.y * 7.0 - t * 1.1),
        cos(uv.x * 14.0 + t * 1.3) + cos(uv.x * 5.0 - t * 0.9)
    ) * 0.0035 * uIntensity;

    vec2 suv = clamp(uv + wobble, vec2(0.001), vec2(0.999));

    // Hafif bulaniklik (merkez + 4 komsu)
    vec2 px = 1.5 / uResolution;
    vec3 col = texture2D(uScene, suv).rgb * 0.4
             + texture2D(uScene, suv + vec2(px.x, 0.0)).rgb * 0.15
             + texture2D(uScene, suv - vec2(px.x, 0.0)).rgb * 0.15
             + texture2D(uScene, suv + vec2(0.0, px.y)).rgb * 0.15
             + texture2D(uScene, suv - vec2(0.0, px.y)).rgb * 0.15;

    // Renk: grilesip mavi-yesil tona kayar
    vec3 tint = vec3(0.35, 0.65, 0.75);
    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(col, lum * tint * 1.3, 0.55 * uIntensity);

    // Isik bantlari
    float caustic = pow(max(0.0, sin(uv.x * 30.0 + t * 2.0) * sin(uv.y * 26.0 - t * 1.7)), 3.0);
    col += caustic * 0.08 * uIntensity * tint;

    // Bulanik su: kenarlar kararir
    vec2 cc = uv - 0.5;
    col *= clamp(1.0 - dot(cc, cc) * 1.2 * uIntensity, 0.0, 1.0);

    gl_FragColor = vec4(col, 1.0);
}