#include "camera.h"

#include "engine/asset_manager/object.h"
#include "engine/engine.h"

void Camera::lateUpdate() {
    const Object& obj = ENGINE().scene.get(object);
    pos = glm::vec3(obj.worldMatrix[3]);

    const glm::vec3 worldUp = glm::vec3(obj.worldMatrix[1]);
    const glm::vec3 worldBack = glm::vec3(obj.worldMatrix[2]);
    const float upLen = glm::length(worldUp);
    const float backLen = glm::length(worldBack);
    if (upLen > 1e-8f)
        up = worldUp / upLen;
    if (backLen > 1e-8f)
        front = -worldBack / backLen;
}
