//
// Created by Martin Wickham on 4/24/17.
//

#ifndef SPONZA_CAMERA_H
#define SPONZA_CAMERA_H

#include "types.h"
#include <glm/glm.hpp>

struct Camera {
    glm::mat4 m_view;
    glm::vec3 m_pos; // should be equivalent to vec3(inverse(m_view)[3])

    virtual void mouseMoved(glm::vec2 delta) = 0;
    virtual void update(s32 dt) = 0;
};

struct OrbitNoGimbleCamera : Camera {
    glm::mat4 m_offset;
    glm::mat4 m_rotation;
    bool m_dirty;

    void mouseMoved(glm::vec2 delta) override;
    void update(s32 dt) override;
};

struct FlyAroundCamera : Camera {
    f32 m_pitchAngle; // in degrees, angle 0 is on xz plane.
    f32 m_yawAngle; // in degrees, angle 0 faces towards -Z.

    void mouseMoved(glm::vec2 delta) override;
    void update(s32 dt) override;
};

#endif //SPONZA_CAMERA_H
