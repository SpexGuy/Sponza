//
// Created by Martin Wickham on 4/24/17.
//

#include <glm/gtx/transform.hpp>
#include "gl_includes.h"
#include "camera.h"

using namespace glm;

void OrbitNoGimbleCamera::mouseMoved(glm::vec2 delta) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) return;
    vec3 rotationVector = vec3(delta.y, delta.x, 0);
    float angle = length(delta);
    m_rotation = rotate(angle, rotationVector) * m_rotation;
}

void OrbitNoGimbleCamera::update(s32 dt) {
    m_view = m_offset * m_rotation;
}
