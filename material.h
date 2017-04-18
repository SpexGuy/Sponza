//
// Created by Martin Wickham on 4/17/17.
//

#ifndef SPONZA_MATERIAL_H
#define SPONZA_MATERIAL_H

#include <glm/glm.hpp>
#include "gl_includes.h"
#include "obj.h"

struct Shader {
    GLuint program;
    void (*bindUniforms)(const glm::mat4 &mvp, const glm::mat3 &normal, const OBJMesh &mesh, const OBJMaterial &material);
};

enum {
    kTexCoord,
    kNormal,
    kDiffuseTex,

    kNumShaders
};


void initShaders();
Shader &findShader(const OBJMesh &mesh, const OBJMaterial &material);
Shader &getShader(int shader);
void bindShader(const Shader &shader);
void bindMaterial(const glm::mat4 &mvp, const glm::mat3 &normal, const OBJMesh &mesh, const OBJMaterial &material);

#endif //SPONZA_MATERIAL_H
