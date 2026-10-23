#version 120

// Su yuzeyi: iki katman doku, farkli yonlere kayar ve sinus dalgasiyla oynar.
// BSP doku koordinati "doku tekrari" biriminde oldugu icin sin(uv * 2pi) tekrar basina bir dalgadir.
uniform sampler2D uTexture;
uniform float uTime;
uniform float uAlpha;   // 1 = opak su, daha azi saydam

void main() {
    vec2 uv = gl_TexCoord[0].st;

    vec2 warp = vec2(
        sin(uv.t * 6.2832 + uTime * 1.4),
        sin(uv.s * 6.2832 + uTime * 1.1)
    ) * 0.025;

    vec3 a = texture2D(uTexture, uv + warp + vec2(uTime * 0.025, uTime * 0.018)).rgb;
    vec3 b = texture2D(uTexture, uv * 1.35 - warp + vec2(-uTime * 0.02, uTime * 0.03)).rgb;

    gl_FragColor = vec4(mix(a, b, 0.4) * gl_Color.rgb, uAlpha);
}