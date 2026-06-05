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
