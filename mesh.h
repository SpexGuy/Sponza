//
// Created by Martin Wickham on 4/19/17.
//

#ifndef SPONZA_MESH_H
#define SPONZA_MESH_H

#include <glm/glm.hpp>
#include <vector>
#include "gl_includes.h"
#include "types.h"

struct Texture {
    u32 glHandle;
};

#define MAT_TRANSPARENCY (1<<0)
#define MAT_AMBIENT_TEX (1<<1)
#define MAT_DIFFUSE_TEX (1<<2)
#define MAT_SPECULAR_TEX (1<<3)
#define MAT_EMISSIVE_TEX (1<<4)
#define MAT_SHININESS_TEX (1<<5)
#define MAT_TRANSPARENCY_TEX (1<<6)
#define MAT_NORMAL_TANGENT_TEX (1<<7)
typedef u16 MaterialFlags;

#define BAD_TEX 0xFFFFFFFF
#define TEX_UNLOADED 0xFFFF

// Generalized lighting calculation is:
// color.rgb = Tf * (
//                 Ke * map_Ke(uv) +
//                 sum for light in lights (
//                     Ka * map_Ka(uv) +
//                     Kd * map_Kd(uv) * diffuseFactor(light) +
//                     Ks * map_Ks(uv) * specularFactor(light)
//                 ));
// color.a = d * map_d(uv);
//
// where
// specularFactor(light) = pow(dot(-eye, reflect(light, normal)), Ns * map_Ns(uv));
// diffuseFactor(light) = dot(light, normal);
// normal = [va_normal|va_tangent|va_bitangent] * map_bump(uv);
struct Material {
    std::string name;     // The tag for the material (optional)
    f32 Ns;               // Shininess
    f32 d;                // Transparency (alpha) (AKA 1-Tr)
    glm::vec3 Tf;         // Transmission filter
    glm::vec3 Ka;         // Ambient reflectivity
    glm::vec3 Kd;         // Diffuse reflectivity
    glm::vec3 Ks;         // Specular reflectivity
    glm::vec3 Ke;         // Emissive reflectivity
    u16 map_Ka;           // Ambient color texture
    u16 map_Kd;           // Diffuse color texture
    u16 map_Ks;           // Specular color texture
    u16 map_Ke;           // Emissive color texture
    u16 map_Ns;           // Shininess texture
    u16 map_d;            // Transparency texture
    u16 map_bump;         // Bump map (normal map)
    MaterialFlags flags;  // MAT_* flags. 1 if field is initialized, 0 otherwise.
};

struct MeshPart {
    u32 offset;    // The first index in the mesh to draw
    u32 size;      // The number of indices in the mesh to draw
    u16 shader;    // The shader to use when drawing this object
    u16 material;  // The material properties to set when drawing this object
};

struct Mesh {
    u32 vao;
    u32 size; // total number of indices
    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<MeshPart> parts;
};



GLuint createVao();

struct OBJMesh;
void obj2mesh(OBJMesh &obj, Mesh &mesh);

#endif //SPONZA_MESH_H
