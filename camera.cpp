//
// Created by Martin Wickham on 4/24/17.
//

#include <glm/gtx/transform.hpp>
#include "gl_includes.h"
#include "camera.h"

using namespace glm;


void OrbitNoGimbleCamera::mouseMoved(glm::vec2 delta) {
    vec3 rotationVector = vec3(delta.y, delta.x, 0);
    float angle = length(delta);
    m_rotation = rotate(angle, rotationVector) * m_rotation;
}

void OrbitNoGimbleCamera::update(s32 dt) {
    m_view = m_offset * m_rotation;
}



const float sensitivity = 0.5;
const float speed = 20;

void FlyAroundCamera::mouseMoved(glm::vec2 delta) {
    m_yawAngle -= delta.x * sensitivity;
    m_pitchAngle = clamp(m_pitchAngle - delta.y * sensitivity, -89.f, 89.f);
}

void FlyAroundCamera::update(s32 dt) {
    float theta = glm::radians(m_yawAngle);
    float phi = glm::radians(m_pitchAngle);

    float cosphi = cos(phi);
    float sinphi = sin(phi);
    float costheta = cos(theta);
    float sintheta = sin(theta);

    vec3 forward = vec3(-cosphi * sintheta, sinphi, -cosphi * costheta);
    vec3 straight = normalize(vec3(forward.x, 0, forward.z));
    vec3 right = vec3(-straight.z, 0, straight.x);

    vec3 movement(0);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        movement.x += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        movement.x -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        movement.y += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        movement.y -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        movement.z += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        movement.z -= 1;
    }

    if (movement != vec3(0)) {
        movement = normalize(movement);
        vec3 velocity = movement.x * straight +
                        movement.y * vec3(0,1,0) +
                        movement.z * right;
        m_position += velocity * speed * f32(dt);
    }

    m_view = lookAt(m_position, m_position + forward, vec3(0,1,0));
}
