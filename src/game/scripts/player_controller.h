#pragma once

#include <cmath>

#include "engine/iscript.h"
#include "engine/engine.h"
#include "engine/asset_manager/object.h"
#include "engine/raycasting.h"
#include "game/world.h"

class PlayerController : public IScript {
public:
    explicit PlayerController(World* world)
        : world(world)
    {
        INSPECT(speed);
        INSPECT(jumpForce);
        INSPECT(lookSensitivity);
    }

    glm::vec3 velocity{0.f};
    bool grounded = false;
    float speed = 1.0f;
    float jumpForce = 3.0f;
    float lookSensitivity = 0.1f;
    float yaw = 0.f;
    float pitch = 0.f;

    void init() override {}
    const char* typeName() const override { return "PlayerController"; }

    void update() override {
        Engine& engine = Engine::instance();
        Object& obj = engine.scene.get(object);
        Input& input = engine.input;
        const float dt = engine.app.deltaTime;

        yaw -= input.mouseDeltaX * lookSensitivity;
        pitch -= input.mouseDeltaY * lookSensitivity;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        const glm::vec3 look = lookDirection();
        const glm::vec3 worldUp(0.f, 1.f, 0.f);
        const glm::vec3 right = glm::normalize(glm::cross(look, worldUp));
        glm::vec3 pos = obj.transform.position();

        const float moveSpeed = speed * dt;
        if (input.down(Action::MoveForward))
            pos += moveSpeed * look;
        if (input.down(Action::MoveBackward))
            pos -= moveSpeed * look;
        if (input.down(Action::MoveLeft))
            pos -= moveSpeed * right;
        if (input.down(Action::MoveRight))
            pos += moveSpeed * right;

        if (input.pressed(Action::Jump) && grounded)
            velocity.y = jumpForce;

        velocity.y += -engine.G * dt;
        pos.y += velocity.y * dt;

        if (world)
            resolveGround(pos);

        obj.transform.setPosition(pos);
        obj.transform.setRotation({pitch, yaw, 0.f});

        clearDebug(engine.scene.getRoot());

        Ray ray{
            .origin = pos,
            .direction = look,
        };
        RaycastHit hit = engine.raycasting.castRay(ray);
        if (hit.object != INVALID_OBJECT_ID)
            engine.scene.get(hit.object).debug = true;
    }

private:
    World* world = nullptr;

    glm::vec3 lookDirection() const {
        return glm::normalize(glm::vec3(
            -sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
             sin(glm::radians(pitch)),
            -cos(glm::radians(yaw)) * cos(glm::radians(pitch))));
    }

    void resolveGround(glm::vec3& pos) {
        const float halfW = (world->width - 1) * world->scale * 0.5f;
        const float halfH = (world->height - 1) * world->scale * 0.5f;

        int x = static_cast<int>(floor((pos.x + halfW) / world->scale));
        int z = static_cast<int>(floor((pos.z + halfH) / world->scale));
        x = glm::clamp(x, 0, world->width - 1);
        z = glm::clamp(z, 0, world->height - 1);

        const int idx = z * world->width + x;
        const float groundY = world->terrain.vertices[idx * 6 + 1];

        if (pos.y <= groundY) {
            velocity.y = 0.f;
            pos.y = groundY;
            grounded = true;
        } else {
            grounded = false;
        }
    }

    static void clearDebug(ObjectID id) {
        Object& obj = Engine::instance().scene.get(id);
        obj.debug = false;
        for (ObjectID child : obj.children)
            clearDebug(child);
    }
};
