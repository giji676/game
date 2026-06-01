#include <glad/glad.h>

#include "material.h"

void Material::bind() const {
    shader->use();

    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string name = textures[i]->type;
        std::string number;

        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);

        else if (name == "texture_specular")
            number = std::to_string(specularNr++);

        shader->setInt(name + number, i);

        glBindTexture(GL_TEXTURE_2D, textures[i]->id);
    }

    shader->setVec3("diffuseFallback", diffuseFallback);
    shader->setVec3("specularFallback", specularFallback);
}
