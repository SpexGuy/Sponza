//
// Created by Martin Wickham on 4/17/17.
//

#ifndef SPONZA_MATERIAL_H
#define SPONZA_MATERIAL_H

#include <glm/glm.hpp>
#include "gl_includes.h"
#include "types.h"
#include "mesh.h"

enum : u16 {
    kTexCoord,
    kNormal,
    kDiffuseTex,

    kNumShaders
};

#define VAO_POS 0
#define VAO_NOR 1
#define VAO_TEX 2

void initShaders();
u16 findShader(const Mesh &mesh, const Material &material);
void bindShader(u16 shader);
void bindMaterial(const glm::mat4 &mvp, const glm::mat3 &normal, const Mesh &mesh, const Material &material);

#endif //SPONZA_MATERIAL_H
