#version 120

// Kara delik: isigin bukulmesi (yercekimsel mercek), olay ufku (siyah disk) ve turuncu birikim
// halkasi. Delik basina bir vec4: xy = ekran uv, z = olay ufku yaricapi (ekran yuksekligi
// biriminde), w = bukme siddeti. CPU tarafi, gorus hatti engelli delikleri yaricapini
// kucultup siddetini azaltarak yumusakca kaybeder.
uniform sampler2D uScene;
uniform float uAspect;
uniform float uTime;
uniform int   uHoleCount;
uniform vec4  uHoles[4];

void main() {
    vec2 uv = gl_TexCoord[0].st;
    vec2 offset = vec2(0.0);
    float horizon = 0.0;
    vec3 glow = vec3(0.0);

    for (int i = 0; i < 4; i++) {
        if (i >= uHoleCount) break;

        vec4 hole = uHoles[i];
        float rs = hole.z;
        if (rs <= 0.0001) continue;

        // Merkeze uzaklik, en-boy orani duzeltilerek (daire bozulmasin)
        vec2 d = uv - hole.xy;
        d.x *= uAspect;
        float r = max(length(d), 0.0001);
        vec2 dir = d / r;

        // Bukme: bu piksel, merkeze daha yakin bir noktanin goruntusunu gosterir (r -> r - bend).
        float reach = rs * 9.0;
        if (r < reach) {
            float bend = hole.w * rs * rs / r;
            bend *= 1.0 - smoothstep(rs * 2.5, reach, r);
            bend = min(bend, r * 0.97);

            vec2 delta = dir * (r - bend) - d;
            delta.x /= uAspect;
            offset += delta;
        }

        // Olay ufku: icerisi siyah
        horizon = max(horizon, 1.0 - smoothstep(rs * 0.92, rs, r));

        // Birikim halkasi: olay ufkunun hemen disinda, donen parlak bantlar
        float q = (r - rs * 1.35) / (rs * 0.16);
        float ring = exp(-q * q);
        float swirl = 0.65 + 0.35 * sin(atan(d.y, d.x) * 3.0 - uTime * 2.5 + r * 40.0);
        glow += vec3(1.0, 0.62, 0.28) * ring * swirl * 0.9 * min(hole.w, 1.5);
    }

    vec2 suv = clamp(uv + offset, vec2(0.0005), vec2(0.9995));
    vec3 col = texture2D(uScene, suv).rgb;
    col *= 1.0 - horizon;
    col += glow;

    gl_FragColor = vec4(col, 1.0);
}