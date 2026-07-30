#include <algorithm>
#include <cstdint>
#include <numeric>

#include "renderer.h"
#include "engine/profilers/profile_scope.h"

void Renderer::init(MeshRegistry* registry) {
    meshRegistry = registry;

    glGenBuffers(NUM_BUFFERS, transformSSBO);
    glGenBuffers(NUM_BUFFERS, indirectBuffer);

    for (uint32_t i = 0; i < NUM_BUFFERS; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, transformSSBO[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     MAX_OBJECTS * sizeof(glm::mat4),
                     nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer[i]);
        glBufferData(GL_DRAW_INDIRECT_BUFFER,
                     MAX_OBJECTS * sizeof(DrawElementsIndirectCommand),
                     nullptr, GL_DYNAMIC_DRAW);
    }
}

void Renderer::cleanup() {
    glDeleteBuffers(NUM_BUFFERS, transformSSBO);
    glDeleteBuffers(NUM_BUFFERS, indirectBuffer);
}

void Renderer::render(std::vector<RenderCommand>& queue,
        const glm::mat4& view,
        const glm::mat4& projection)
{
    PROFILE_SCOPE("Renderer::render");

    // Swap to next buffer
    currentBuffer = (currentBuffer + 1) % NUM_BUFFERS;
    GLuint currentTransformSSBO  = transformSSBO[currentBuffer];
    GLuint currentIndirectBuffer = indirectBuffer[currentBuffer];

    {
        PROFILE_SCOPE("Renderer::sort");
        sortedIndices.resize(queue.size());
        std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
        std::sort(sortedIndices.begin(), sortedIndices.end(),
                  [&](uint32_t a, uint32_t b) {
                      return queue[a].sortKey < queue[b].sortKey;
                  });
    }

    {
        PROFILE_SCOPE("Renderer::main");
        // Build indirect commands and transform buffer
        {
            PROFILE_SCOPE("Renderer::build_buffers");

            transformData.resize(queue.size());
            commands.resize(queue.size());
            {
                PROFILE_SCOPE("Renderer::build_arrays");
                for (uint32_t i = 0; i < queue.size(); i++) {
                    const RenderCommand& cmd = queue[sortedIndices[i]];
                    const MeshAllocation& alloc = *cmd.allocation;

                    transformData[i] = cmd.model;

                    commands[i].count         = alloc.indexCount;
                    commands[i].instanceCount = 1;
                    commands[i].firstIndex    = alloc.firstIndex;
                    commands[i].baseVertex    = alloc.baseVertex;
                    commands[i].baseInstance  = i;
                }
            }
            {
                PROFILE_SCOPE("Renderer::upload_transforms");
                // transform SSBO
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, currentTransformSSBO);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                        transformData.size() * sizeof(glm::mat4),
                        transformData.data());
            }

            {
                PROFILE_SCOPE("Renderer::upload_commands");
                // indirect buffer
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, currentIndirectBuffer);
                glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
                        commands.size() * sizeof(DrawElementsIndirectCommand),
                        commands.data());
            }
        }

        // Draw groups by material
        {
            PROFILE_SCOPE("Renderer::draw");

            glBindVertexArray(meshRegistry->VAO);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, currentTransformSSBO);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, currentIndirectBuffer);

            Shader* currentShader = nullptr;
            const Material* currentMaterial = nullptr;
            uint32_t groupStart = 0;

            for (uint32_t i = 0; i <= queue.size(); i++) {
                bool flush = (i == queue.size());

                if (!flush) {
                    const RenderCommand& cmd = queue[sortedIndices[i]];
                    flush = flush
                        || (cmd.material->shader != currentShader)
                        || (cmd.material != currentMaterial);
                }

                if (flush && i > 0) {
                    PROFILE_SCOPE("Renderer::multi_draw");
                    glMultiDrawElementsIndirect(
                            GL_TRIANGLES,
                            GL_UNSIGNED_INT,
                            (void*)(groupStart * sizeof(DrawElementsIndirectCommand)),
                            i - groupStart,
                            0
                            );
                    groupStart = i;
                }

                if (i == queue.size()) break;
                const RenderCommand& cmd = queue[sortedIndices[i]];
                if (cmd.material->shader != currentShader) {
                    PROFILE_SCOPE("Renderer::shader_change");
                    currentShader = cmd.material->shader;
                    currentShader->use();
                    currentShader->setMat4("view", view);
                    currentShader->setMat4("projection", projection);
                    currentMaterial = nullptr;
                }

                if (cmd.material != currentMaterial) {
                    PROFILE_SCOPE("Renderer::material_change");
                    currentMaterial = cmd.material;
                    currentMaterial->bind();

                    bool hasDiffuse = false;
                    bool hasSpecular = false;
                    for (Texture* tex : currentMaterial->textures) {
                        if (!tex || tex->id == 0)
                            continue;
                        if (tex->type == "diffuseMap")
                            hasDiffuse = true;
                        else if (tex->type == "specularMap")
                            hasSpecular = true;
                    }

                    currentShader->setVec3("diffuseFallback",
                            currentMaterial->diffuseFallback);
                    currentShader->setVec3("specularFallback",
                            currentMaterial->specularFallback);
                    currentShader->setBool("hasDiffuseMap", hasDiffuse);
                    currentShader->setBool("hasSpecularMap", hasSpecular);
                    currentShader->setInt("diffuseMap0", 0);
                    currentShader->setInt("specularMap0", 1);
                }
            }
        }
    }
}
