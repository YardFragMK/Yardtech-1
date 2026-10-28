#version 120

// Parildayan enerji kuresi: parlak cekirdek, yumusak hale, donen yildiz isinlari ve titresim.
// Ek (additive) karisimla cizilir; cikti dogrudan parlakliktir. Doku koordinati -1..1.
uniform vec3  uColor;
uniform float uTime;
uniform float uSeed;     // her top icin farkli faz
uniform float uSpikes;   // yildiz isini sayisi

void main() {
    vec2 p = gl_TexCoord[0].st;
    float r = length(p);

    float core = exp(-r * r * 22.0);
    float halo = exp(-r * r * 4.0) * 0.55;

    float a = atan(p.y, p.x);
    float spin = uTime * 2.2 + uSeed * 6.2831;
    float rays = pow(abs(cos(a * uSpikes * 0.5 + spin)), 28.0)
               + 0.6 * pow(abs(cos(a * uSpikes * 0.5 - spin * 0.7 + 1.0)), 40.0);
    rays *= exp(-r * 2.4) * (0.65 + 0.35 * sin(uTime * 18.0 + uSeed * 40.0));

    float twinkle = 0.85 + 0.15 * sin(uTime * 31.0 + uSeed * 17.0 + r * 9.0);
    vec3 col = uColor * (halo + rays) * twinkle + vec3(1.0) * core;

    // Dortgenin kenarinda sert kesilme olmasin.
    col *= 1.0 - smoothstep(0.85, 1.0, r);

    gl_FragColor = vec4(col, 1.0);
}