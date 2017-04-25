//
// Created by Martin Wickham on 4/24/17.
//

#ifndef SPONZA_CAMERA_H
#define SPONZA_CAMERA_H

#include "types.h"
#include <glm/glm.hpp>

struct Camera {
    glm::mat4 m_view;

    virtual void mouseMoved(glm::vec2 delta) = 0;
    virtual void update(s32 dt) = 0;
};

struct OrbitNoGimbleCamera : Camera {
    glm::mat4 m_offset;
    glm::mat4 m_rotation;

    void mouseMoved(glm::vec2 delta) override;
    void update(s32 dt) override;
};

struct FlyAroundCamera : Camera {
    f32 m_pitchAngle; // in degrees, angle 0 is on xz plane.
    f32 m_yawAngle; // in degrees, angle 0 faces towards -Z.
    glm::vec3 m_position;

    void mouseMoved(glm::vec2 delta) override;
    void update(s32 dt) override;
};

#endif //SPONZA_CAMERA_H
