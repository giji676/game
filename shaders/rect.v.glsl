#version 330 core

layout(location=0) in vec2 unitPos;
layout(location=1) in vec2 iPos;
layout(location=2) in vec2 iSize;
layout(location=3) in vec4 iColor;
layout(location=4) in vec4 iBorderColor;
layout(location=5) in vec4 iRadii;
layout(location=6) in float iBorderWidth;

uniform mat4 uProjection;

out vec2 vLocalPos;
out vec2 vHalfSize;
out vec4 vColor;
out vec4 vBorderColor;
out vec4 vRadii;
out float vBorderWidth;

void main() {
    vec2 worldPos = iPos + unitPos * iSize;
    vLocalPos = (unitPos - 0.5) * iSize; // centered coords for the SDF
    vHalfSize = iSize * 0.5;
    vColor = iColor;
    vBorderColor = iBorderColor;
    vRadii = iRadii;
    vBorderWidth = iBorderWidth;
    gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
}
