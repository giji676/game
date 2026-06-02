#include <glad/glad.h>

#include "material.h"

void Material::bind() const {
    // shader->use();

    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string type = textures[i]->type;
        std::string number;

        if (type == "diffuseMap")
            number = std::to_string(diffuseNr++);

        else if (type == "specularMap")
            number = std::to_string(specularNr++);

        shader->setInt(type + number, i);

        glBindTexture(GL_TEXTURE_2D, textures[i]->id);
    }

    shader->setVec3("diffuseFallback", diffuseFallback);
    shader->setVec3("specularFallback", specularFallback);
}
