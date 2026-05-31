#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <vector>
#include "shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    // mesh data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    glm::vec3 diffuseFallback = glm::vec3(1.0f);
    glm::vec3 specularFallback = glm::vec3(0.0f);
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
          std::vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        setupMesh();
    }
    void Draw(Shader &shader)
    {
        int numDiffuse = 0;
        int numSpecular = 0;

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            if (textures[i].type == "texture_diffuse")
                numDiffuse++;
            else if (textures[i].type == "texture_specular")
                numSpecular++;
        }

        shader.setInt("numDiffuse", numDiffuse);
        shader.setInt("numSpecular", numSpecular);
        shader.setVec3("diffuseFallback", diffuseFallback);
        shader.setVec3("specularFallback", specularFallback);

        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;

        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);

            std::string number;
            std::string name = textures[i].type;

            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++);

            shader.setInt((name + number).c_str(), i);

            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
private:
    // render data
    unsigned int VAO, VBO, EBO;
    void setupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                     &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() *
                     sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, TexCoords));
        glBindVertexArray(0);
    }
};
