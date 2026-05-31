#include "renderer.h"


void Renderer::draw(const Object &object) {
    object.model->draw();
}
