#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct DebugVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

struct DebugLine {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
    bool depthTest = true;
};

class DebugRenderer {
public:
    void init();
    void upload();

    void makeVertices(const std::vector<DebugLine>& lines);

    void render(
        const glm::mat4& view,
        const glm::mat4& projection);

    void line(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color,
        bool depthTest = true);

    void arrow(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float length,
        const glm::vec3& color,
        bool depthTest = true);

    void quadOutline(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d,
        const glm::vec3& color,
        bool depthTest = true);

    void box(
        const glm::mat4& world,
        const glm::vec3& size,
        const glm::vec3& color);

    void aabb(
        const glm::mat4& world,
        const glm::vec3& min,
        const glm::vec3& max,
        const glm::vec3& color);

    void axis(
        const glm::mat4& world,
        float size = 1.0f);

    void clear();

    const std::vector<DebugLine>& lines() const {
        return lines_;
    }

private:
    std::vector<DebugLine> lines_;
    std::vector<DebugVertex> vertices_;

    GLuint vao;
    GLuint vbo;
};
