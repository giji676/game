#include "debug_renderer.h"
#include "engine/asset_manager/shader.h"
#include "engine/engine.h"
#include "engine/profilers/profile_scope.h"

void DebugRenderer::aabb(
    const glm::mat4& world,
    const glm::vec3& min,
    const glm::vec3& max,
    const glm::vec3& color)
{
    glm::vec3 p[8] = {
        {min.x,min.y,min.z},
        {max.x,min.y,min.z},
        {max.x,max.y,min.z},
        {min.x,max.y,min.z},

        {min.x,min.y,max.z},
        {max.x,min.y,max.z},
        {max.x,max.y,max.z},
        {min.x,max.y,max.z}
    };

    for (int i = 0; i < 8; i++) {
        p[i] = glm::vec3(world * glm::vec4(p[i], 1.0f));
    }

    line(p[0], p[1], color);
    line(p[1], p[2], color);
    line(p[2], p[3], color);
    line(p[3], p[0], color);

    line(p[4], p[5], color);
    line(p[5], p[6], color);
    line(p[6], p[7], color);
    line(p[7], p[4], color);

    line(p[0], p[4], color);
    line(p[1], p[5], color);
    line(p[2], p[6], color);
    line(p[3], p[7], color);
}

void DebugRenderer::box(
    const glm::mat4& world,
    const glm::vec3& size,
    const glm::vec3& color)
{
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 vertices[8] = {
        {-halfSize.x, -halfSize.y, -halfSize.z},
        { halfSize.x, -halfSize.y, -halfSize.z},
        { halfSize.x,  halfSize.y, -halfSize.z},
        {-halfSize.x,  halfSize.y, -halfSize.z},
        {-halfSize.x, -halfSize.y,  halfSize.z},
        { halfSize.x, -halfSize.y,  halfSize.z},
        { halfSize.x,  halfSize.y,  halfSize.z},
        {-halfSize.x,  halfSize.y,  halfSize.z}
    };
    unsigned int indices[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };
    for (unsigned int i = 0; i < 24; i += 2) {
        line(
            world * glm::vec4(vertices[indices[i]],   1.0f),
            world * glm::vec4(vertices[indices[i+1]], 1.0f),
            color
        );
    }
}

void DebugRenderer::render(
    const glm::mat4& view,
    const glm::mat4& projection)
{
    PROFILE_SCOPE("DebugRenderer::render");

    Shader& shader = ENGINE().assets.getShader("debug");
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glBindVertexArray(vao);

    auto drawPass = [&](bool depthTest) {
        vertices_.clear();
        for (const DebugLine& line : lines_) {
            if (line.depthTest != depthTest)
                continue;
            vertices_.push_back({line.start, line.color});
            vertices_.push_back({line.end, line.color});
        }
        if (vertices_.empty())
            return;

        if (depthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            vertices_.size() * sizeof(DebugVertex),
            vertices_.data(),
            GL_DYNAMIC_DRAW
        );

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices_.size()));
    };

    drawPass(true);
    drawPass(false);

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
    const glm::vec3& color,
    bool depthTest)
{
    lines_.push_back({
        start,
        end,
        color,
        depthTest
    });
}

void DebugRenderer::arrow(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float length,
    const glm::vec3& color,
    bool depthTest)
{
    if (length <= 0.f)
        return;

    const float dirLen2 = glm::dot(direction, direction);
    if (dirLen2 < 1e-12f)
        return;

    const glm::vec3 dir = direction * (1.f / std::sqrt(dirLen2));
    const glm::vec3 tip = origin + dir * length;
    const float tipLen = length * 0.2f;
    const glm::vec3 shaftEnd = tip - dir * tipLen;

    line(origin, tip, color, depthTest);

    glm::vec3 up = (std::abs(dir.y) < 0.99f)
        ? glm::vec3(0.f, 1.f, 0.f)
        : glm::vec3(1.f, 0.f, 0.f);
    const glm::vec3 right = glm::normalize(glm::cross(dir, up));
    const glm::vec3 side = right * (length * 0.08f);

    line(tip, shaftEnd + side, color, depthTest);
    line(tip, shaftEnd - side, color, depthTest);
}

void DebugRenderer::quadOutline(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& d,
    const glm::vec3& color,
    bool depthTest)
{
    line(a, b, color, depthTest);
    line(b, c, color, depthTest);
    line(c, d, color, depthTest);
    line(d, a, color, depthTest);
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
