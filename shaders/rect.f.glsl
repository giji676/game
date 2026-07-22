#version 330 core

in vec2 vLocalPos;
in vec2 vHalfSize;
in vec4 vColor;
in vec4 vBorderColor;
in vec4 vRadii;      // TL, TR, BR, BL
in float vBorderWidth;
out vec4 fragColor;

float sdRoundBox(vec2 p, vec2 halfSize, vec4 r) {
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x  = (p.y > 0.0) ? r.x  : r.y;
    vec2 q = abs(p) - halfSize + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

void main() {
    float dist = sdRoundBox(vLocalPos, vHalfSize, vRadii);
    float aa = fwidth(dist);

    float outerAlpha = 1.0 - smoothstep(-aa, aa, dist);
    if (outerAlpha <= 0.0) discard;

    vec4 color = vColor;
    if (vBorderWidth > 0.0) {
        float innerDist = dist + vBorderWidth;
        float innerAlpha = 1.0 - smoothstep(-aa, aa, innerDist);
        color = mix(vBorderColor, vColor, innerAlpha);
    }
    color.a *= outerAlpha;
    fragColor = color;
}
