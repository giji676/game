#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(std430, binding = 0) readonly buffer Transforms {
    mat4 transforms[];
};

uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    mat4 model = transforms[gl_BaseInstance];
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;
    FragPos = worldPos.xyz;
    // TODO: procompute the normal instead of every frame/vertex
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}
