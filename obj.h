//
// Created by Martin Wickham on 4/10/17.
//

#ifndef SPONZA_OBJ_H
#define SPONZA_OBJ_H

#include "types.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

// material flags
#define OBJ_MTL_NS (1<<0)
#define OBJ_MTL_NI (1<<1)
#define OBJ_MTL_D (1<<2)
#define OBJ_MTL_TF (1<<3)
#define OBJ_MTL_ILLUM (1<<4)
#define OBJ_MTL_KA (1<<5)
#define OBJ_MTL_KD (1<<6)
#define OBJ_MTL_KS (1<<7)
#define OBJ_MTL_KE (1<<8)
#define OBJ_MTL_MAP_KA (1<<9)
#define OBJ_MTL_MAP_KD (1<<10)
#define OBJ_MTL_MAP_KS (1<<11)
#define OBJ_MTL_MAP_KE (1<<12)
#define OBJ_MTL_MAP_NS (1<<13)
#define OBJ_MTL_MAP_D (1<<14)
#define OBJ_MTL_MAP_BUMP (1<<15)
typedef u32 MaterialFlags;

struct OBJMaterial {
    std::string name;     // The tag for the material, after newmtl
    f32 Ns;               // Shininess
    f32 Ni;               // Index of refraction
    f32 d;                // Transparency (alpha) (AKA 1-Tr)
    glm::vec3 Tf;         // Transmission filter
    u16 illum;            // Illumination model (https://en.wikipedia.org/wiki/Wavefront_.obj_file#Basic_materials)
    glm::vec3 Ka;         // Ambient reflectivity. `Ka xyz` and `Ka spectral` are not supported.
    glm::vec3 Kd;         // Diffuse reflectivity. `Kd xyz` and `Kd spectral` are not supported.
    glm::vec3 Ks;         // Specular reflectivity. `Ks xyz` and `Ks spectral` are not supported.
    glm::vec3 Ke;         // Emissive reflectivity. `Ke xyz` and `Ke spectral` are not supported.
    std::string map_Ka;   // Ambient color texture
    std::string map_Kd;   // Diffuse color texture
    std::string map_Ks;   // Specular color texture
    std::string map_Ke;   // Emissive color texture
    std::string map_Ns;   // Shininess texture
    std::string map_d;    // Transparency texture
    std::string map_bump; // Bump map (normal map)
    MaterialFlags flags;  // OBJ_MTL_* flags. 1 if field is initialized, 0 otherwise.
};

bool loadMaterials(const std::string &filename, std::vector<OBJMaterial> &materials);

#endif //SPONZA_OBJ_H