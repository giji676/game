#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightColor;
uniform vec3 lightPos;

void main() {
    bool isFlat = FragPos.y <= 0.0;
    vec3 color = vec3(0.9, 0.0, 0.5);
    if (isFlat) color = vec3(0.0, 1.0, 0.2);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 ambient = 0.2 * lightColor;

    vec3 lighting = ambient + diffuse;

    FragColor = vec4(color * lighting, 1.0);
}
