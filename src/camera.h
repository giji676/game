#ifndef GJ_CAMERA_H
#define GJ_CAMERA_H

#include "glm/ext/vector_float3.hpp"

class GJ_Camera {
public:
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 direction;
    float yaw = -90.f;
    float pitch;
private:
};

#endif
