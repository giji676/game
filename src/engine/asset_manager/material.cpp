#include <glad/glad.h>

#include "material.h"

void Material::bind() const {
    // Fixed texture units:
    // 0 = diffuseMap0
    // 1 = specularMap0
    // 2 = alphaMap0
    GLuint diffuseId = 0;
    GLuint specularId = 0;
    GLuint alphaId = 0;

    for (Texture* tex : textures) {
        if (!tex)
            continue;
        if (tex->type == "diffuseMap")
            diffuseId = tex->id;
        else if (tex->type == "specularMap")
            specularId = tex->id;
        else if (tex->type == "alphaMap")
            alphaId = tex->id;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, alphaId);
}

bool Material::usesTransparency() const {
    if (opacity < 0.999f)
        return true;
    for (Texture* tex : textures) {
        if (tex && tex->id != 0 && tex->type == "alphaMap")
            return true;
    }
    return false;
}
