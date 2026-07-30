#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightColor;
uniform vec3 lightPos;

uniform sampler2D diffuseMap0;
uniform sampler2D specularMap0;
uniform sampler2D alphaMap0;
uniform bool hasDiffuseMap;
uniform bool hasSpecularMap;
uniform bool hasAlphaMap;
uniform vec3 diffuseFallback;
uniform vec3 specularFallback;
uniform float opacity;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 texColor = hasDiffuseMap
        ? texture(diffuseMap0, TexCoord).rgb
        : diffuseFallback;

    float alpha = opacity;
    if (hasAlphaMap) {
        // Typical MTL map_d textures are grayscale; opacity lives in R.
        alpha *= texture(alphaMap0, TexCoord).r;
    }

    vec3 ambient = 0.2 * lightColor;
    vec3 diffuse = diff * lightColor;

    vec3 lighting = ambient + diffuse;

    FragColor = vec4(texColor * lighting, alpha);
}
