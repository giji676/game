#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

class Transform {
public:
    const glm::vec3& position() const { return position_; }
    const glm::vec3& rotation() const { return rotation_; }
    const glm::vec3& scale() const { return scale_; }

    void setPosition(const glm::vec3& p) {
        position_ = p;
        dirty_ = true;
    }

    void setRotation(const glm::vec3& r) {
        rotation_ = r;
        dirty_ = true;
    }

    void rotate(const glm::vec3& delta) {
        rotation_ += delta;
        dirty_ = true;
    }

    void setScale(const glm::vec3& s) {
        scale_ = s;
        dirty_ = true;
    }

    bool isDirty() const {
        return dirty_;
    }

    void rebuildLocalMatrix() const {
        localMatrix_ = transformToMatrix(
            position_,
            rotation_,
            scale_);
        dirty_ = false;
    }

    const glm::mat4& localMatrix() const {
        if (dirty_) rebuildLocalMatrix();

        return localMatrix_;
    }

private:
    glm::vec3 position_{0.f};
    glm::vec3 rotation_{0.f};
    glm::vec3 scale_{1.f};

    // TODO: cache world pos as well
    mutable glm::mat4 localMatrix_{1.f};

    mutable bool dirty_ = true;

    void clearDirty() {
        dirty_ = false;
    }

    static glm::mat4 transformToMatrix(
        const glm::vec3& pos,
        const glm::vec3& rot,
        const glm::vec3& scale)
    {
        glm::mat4 m(1.0f);

        m = glm::translate(m, pos);

        m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1,0,0));
        m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0,1,0));
        m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0,0,1));

        m = glm::scale(m, scale);

        return m;
    }
};
