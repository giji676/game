#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightColor;
uniform vec3 lightPos;

uniform sampler2D diffuseMap0;
uniform sampler2D specularMap0;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 texColor = texture(diffuseMap0, TexCoord).rgb;

    vec3 ambient = 0.2 * lightColor;
    vec3 diffuse = diff * lightColor;

    vec3 lighting = ambient + diffuse;

    FragColor = vec4(texColor * lighting, 1.0);
}
