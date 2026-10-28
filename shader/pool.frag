#version 120

// Lav ve asit havuzu: iki katman akan sivi dokusu, duzensiz kenar maskesi ile kesilir.
// Lavda kenarlar sogumus kabuk gibi kararir ve havuz nabiz atar; asitte kenarlarda kopuk
// ve hafif titresim vardir.
uniform sampler2D uLiquid;   // kareli tekrar eden sivi dokusu
uniform sampler2D uMask;     // duzensiz havuz kenari (alfa)
uniform float uTime;
uniform float uAlpha;
uniform int   uMode;         // 0 lav, 1 asit

void main() {
    vec2 uv = gl_TexCoord[0].st;
    float mask = texture2D(uMask, uv).a;

    float speed = (uMode == 0) ? 0.5 : 1.0;
    vec2 flow = vec2(uTime * 0.05, uTime * 0.03) * speed;
    vec2 warp = vec2(sin(uv.y * 9.0 + uTime * 1.3), sin(uv.x * 8.0 + uTime * 1.1)) * 0.02;

    vec3 a = texture2D(uLiquid, uv * 1.5 + flow + warp).rgb;
    vec3 b = texture2D(uLiquid, uv * 2.3 - flow * 0.8 - warp).rgb;
    vec3 col = mix(a, b, 0.45);

    float edge = smoothstep(0.0, 0.7, mask);

    if (uMode == 0) {
        col = mix(col * vec3(0.35, 0.18, 0.12), col, edge);          // kenarda kabuk
        col *= 1.0 + 0.25 * sin(uTime * 2.0 + uv.x * 6.0 + uv.y * 5.0); // nabiz
    }
    else {
        float foam = 1.0 - smoothstep(0.15, 0.55, mask);
        col += vec3(0.5, 0.9, 0.35) * foam * 0.5;                    // kenarda kopuk
        col *= 1.0 + 0.2 * sin(uTime * 3.0 + uv.x * 8.0);
    }

    gl_FragColor = vec4(col, mask * uAlpha);
}