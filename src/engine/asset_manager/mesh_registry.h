#pragma once
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <glad/glad.h>
#include "mesh.h"

struct MeshAllocation {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t baseVertex;
    uint32_t meshId;
};

class MeshRegistry {
public:
    void init() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)0);
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, Normal));
        // texcoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }

    void addMesh(const Mesh* mesh) {
        MeshAllocation alloc;
        alloc.firstIndex  = totalIndices;
        alloc.indexCount  = mesh->indices.size();
        alloc.baseVertex  = totalVertices;
        alloc.meshId      = meshes.size(); // index of this mesh

        totalVertices += mesh->vertices.size();
        totalIndices  += mesh->indices.size();

        allocations[mesh] = alloc;
        meshes.push_back(mesh);
    }

    void uploadToGPU() {
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     totalVertices * sizeof(Vertex),
                     nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     totalIndices * sizeof(uint32_t),
                     nullptr, GL_STATIC_DRAW);

        for (const Mesh* mesh : meshes) {
            const MeshAllocation& alloc = allocations[mesh];

            glBufferSubData(GL_ARRAY_BUFFER,
                            alloc.baseVertex * sizeof(Vertex),
                            mesh->vertices.size() * sizeof(Vertex),
                            mesh->vertices.data());

            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                            alloc.firstIndex * sizeof(uint32_t),
                            mesh->indices.size() * sizeof(uint32_t),
                            mesh->indices.data());
        }

        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    const MeshAllocation& getAllocation(const Mesh* mesh) {
        auto it = allocations.find(mesh);
        if (it == allocations.end()) {
            throw std::runtime_error("Mesh not registered in MeshRegistry — did you forget to call addMesh?");
        }
        return it->second;
    }

    GLuint VAO, VBO, EBO;

private:
    std::unordered_map<const Mesh*, MeshAllocation> allocations;
    std::vector<const Mesh*> meshes;
    uint32_t totalVertices = 0;
    uint32_t totalIndices  = 0;
};
