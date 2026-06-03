#include <glad/glad.h>

#include "material.h"

void Material::bind() const {
    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]->id);
    }
}
