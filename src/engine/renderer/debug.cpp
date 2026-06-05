#include "debug.h"
#include "engine/asset_manager/shader.h"
#include "engine/engine.h"

void DebugRenderer::render(
    const glm::mat4& view,
    const glm::mat4& projection)
{
    upload();

    Shader& shader = Engine::instance().assets.getShader("debug");
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glBindVertexArray(vao);

    glDrawArrays(
        GL_LINES,
        0,
        vertices_.size()
    );

    clear();
}

void DebugRenderer::init() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(DebugVertex),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(DebugVertex),
        (void*)offsetof(DebugVertex, color)
    );
    glEnableVertexAttribArray(1);
}

void DebugRenderer::upload() {
    vertices_.clear();
    makeVertices(lines_);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices_.size() * sizeof(DebugVertex),
        vertices_.data(),
        GL_DYNAMIC_DRAW
    );
}

void DebugRenderer::makeVertices(const std::vector<DebugLine>& lines) {
    for (const DebugLine& line : lines) {
        vertices_.push_back({line.start, line.color});
        vertices_.push_back({line.end,   line.color});
    }
}

void DebugRenderer::line(
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& color)
{
    lines_.push_back({
        start,
        end,
        color
    });
}

void DebugRenderer::axis(
    const glm::mat4& world,
    float size)
{
    glm::vec3 origin = world * glm::vec4(0, 0, 0, 1);
    glm::vec3 xAxis  = world * glm::vec4(size, 0, 0, 1);
    glm::vec3 yAxis  = world * glm::vec4(0, size, 0, 1);
    glm::vec3 zAxis  = world * glm::vec4(0, 0, size, 1);

    line(origin, xAxis, {1.f, 0.f, 0.f});
    line(origin, yAxis, {0.f, 1.f, 0.f});
    line(origin, zAxis, {0.f, 0.f, 1.f});
}

void DebugRenderer::clear() {
    lines_.clear();
    vertices_.clear();
}
