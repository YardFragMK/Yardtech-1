#version 120

// Yuzeyin on yuz normali NORMAL_SIGN ile carpilir. Isik yuzeylerin ARKA tarafinda
// gorunuyorsa bu degeri -1.0 yap ve konsolda shader_reload yaz (yeniden derlemek gerekmez).
const float NORMAL_SIGN = 1.0;
const int MAX_LIGHTS = 8;

uniform sampler2D uDiffuse;
uniform sampler2D uLightmap;
uniform int  uHasLightmap;
uniform int  uLightCount;
uniform vec4 uLightPos[MAX_LIGHTS];    // xyz: goz uzayi konumu, w: yaricap
uniform vec4 uLightColor[MAX_LIGHTS];  // rgb: renk * siddet

varying vec3 vEyePos;
varying vec3 vNormal;

void main() {
    vec4 tex = texture2D(uDiffuse, gl_TexCoord[0].st);
    vec3 lm = (uHasLightmap != 0) ? texture2D(uLightmap, gl_TexCoord[1].st).rgb : vec3(0.5);

    // Sabit fonksiyonlu yolla ayni: doku * isik haritasi * 2 (overbright)
    vec3 color = tex.rgb * lm * 2.0;

    vec3 n = normalize(vNormal) * NORMAL_SIGN;
    vec3 dynamicLight = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (i >= uLightCount) break;

        vec3 toLight = uLightPos[i].xyz - vEyePos;
        float dist = length(toLight);
        float radius = uLightPos[i].w;
        if (dist >= radius) continue;

        // Yumusak (kuadratik) dusus; yuzeyin arkasindaki isik etkisiz kalir. Bu, ince
        // bir duvarin diger tarafina isik sizmasini onler.
        float falloff = 1.0 - dist / radius;
        falloff *= falloff;
        float facing = max(dot(n, toLight / max(dist, 0.001)), 0.0);

        dynamicLight += uLightColor[i].rgb * falloff * facing;
    }

    color += tex.rgb * dynamicLight;
    gl_FragColor = vec4(color, tex.a);
}