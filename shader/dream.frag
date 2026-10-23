#version 120

// Ruya: dalgali bozulma, merkeze dogru nabiz atan radyal bulaniklik, soguk
// renk, bloom, vinyet ve onceki kareyle karisim (hayalet/iz). Vinyet ve renk yalnizca yeni kareye
// uygulanir; aksi halde onceki kareyle karisinca her karede birikip kenarlar tamamen kararirdi.
uniform sampler2D uScene;
uniform sampler2D uHistory;
uniform vec2  uResolution;
uniform float uTime;
uniform float uIntensity;   // 0..1

void main() {
    vec2 uv = gl_TexCoord[0].st;
    float I = uIntensity;
    vec2 center = vec2(0.5);

    // Dalgali bozulma
    vec2 wave = vec2(sin(uv.y * 9.0 + uTime * 0.9), cos(uv.x * 8.0 + uTime * 0.7)) * 0.006 * I;
    vec2 suv = clamp(uv + wave, vec2(0.001), vec2(0.999));

    // Radyal (zoom) bulaniklik, nabiz gibi artip azalir
    float pulse = 0.6 + 0.4 * sin(uTime * 1.3);
    vec2 dir = center - suv;
    vec3 blur = vec3(0.0);
    float total = 0.0;
    const int TAPS = 10;
    for (int i = 0; i < TAPS; i++) {
        float t = float(i) / float(TAPS - 1);
        float w = 1.0 - t * 0.6;
        blur += texture2D(uScene, suv + dir * t * 0.06 * pulse * I).rgb * w;
        total += w;
    }
    blur /= total;

    // Soguk, soluk renk ve hafif kontrast artisi
    float lum = dot(blur, vec3(0.299, 0.587, 0.114));
    vec3 fresh = mix(blur, lum * vec3(0.78, 0.85, 1.15), 0.55 * I);
    fresh = (fresh - 0.5) * (1.0 + 0.1 * I) + 0.5;

    // Bloom: parlak alanlar etrafa tasar
    vec3 bloom = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785398;
        vec2 o = vec2(cos(a), sin(a)) * 0.012;
        bloom += max(texture2D(uScene, suv + o).rgb - 0.55, 0.0);
    }
    fresh += bloom * 0.18 * I;

    // Vinyet (yalnizca yeni kareye)
    vec2 cc = uv - center;
    fresh *= clamp(1.0 - dot(cc, cc) * 1.6 * I, 0.0, 1.0);

    // Onceki kare hafifce buyutulerek okunur: tunel gibi iz birakir.
    vec2 huv = cc * 0.993 + center;
    vec3 history = texture2D(uHistory, huv).rgb;

    gl_FragColor = vec4(mix(fresh, history, 0.55 * I), 1.0);
}