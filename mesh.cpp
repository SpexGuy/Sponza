//
// Created by Martin Wickham on 4/19/17.
//

#include <algorithm>

#include "mesh.h"
#include "obj.h"
#include "material.h"

using namespace std;
using namespace glm;

GLuint createVao() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    checkError();

    GLuint verts, indices;
    glGenBuffers(1, &verts);
    glGenBuffers(1, &indices);
    glBindBuffer(GL_ARRAY_BUFFER, verts);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);

    glEnableVertexAttribArray(VAO_POS);
    glEnableVertexAttribArray(VAO_NOR);
    glEnableVertexAttribArray(VAO_TEX);
    glVertexAttribPointer(VAO_POS, 3, GL_FLOAT, GL_FALSE, sizeof(OBJVertex), 0);
    glVertexAttribPointer(VAO_NOR, 3, GL_FLOAT, GL_FALSE, sizeof(OBJVertex), (void *) sizeof(vec3));
    glVertexAttribPointer(VAO_TEX, 2, GL_FLOAT, GL_FALSE, sizeof(OBJVertex), (void *) (sizeof(vec3) * 2));
    checkError();

    return vao;
}

void optimizeMesh(vector<MeshPart> &parts, vector<u32> &indices) {
    if (parts.size() == 0)
        return; // shouldn't happen but JIC

    stable_sort(parts.begin(), parts.end(),
                [](const MeshPart &a, const MeshPart &b) {
                    return a.material < b.material;
                });

    vector<u32> newIndices(indices.size());
    vector<MeshPart> newMeshParts;
    u16 currentMaterial = parts[0].material;
    u32 pos = 0;
    newMeshParts.push_back(MeshPart {pos, 0, 0, currentMaterial});
    for (MeshPart &part : parts) {
        u16 mat = part.material;
        if (mat != currentMaterial) {
            newMeshParts.back().size = pos - newMeshParts.back().offset;
            currentMaterial = mat;
            newMeshParts.push_back(MeshPart {pos, 0, 0, currentMaterial});
        }
        memcpy(&newIndices[pos], &indices[part.offset], part.size * sizeof(u32));
        pos += part.size;
    }
    assert(pos == indices.size());
    newMeshParts.back().size = pos - newMeshParts.back().offset;

    printf("Mesh optimized; number of parts reduced from %lu to %lu\n", parts.size(), newMeshParts.size());

    parts = std::move(newMeshParts);
    indices = std::move(newIndices);
}

void obj2mesh_texture(const OBJTexture &obj, Texture &tex) {
    tex.glHandle = obj.texName;
}

void obj2mesh_material(const OBJMaterial &obj, Material &mat) {
    #define prop(flag, var, fallback, matflag) \
        do { if (obj.flags & flag) { mat.var = obj.var; mat.flags |= (matflag); } else { mat.var = (fallback); } } while (0)

    mat.name = obj.name;
    mat.flags = 0;

    prop(OBJ_MTL_NS, Ns, 1.f, 0);
    prop(OBJ_MTL_D, d, 1.f, 0);
    prop(OBJ_MTL_TF, Tf, vec3(1), 0);
    prop(OBJ_MTL_KA, Ka, vec3(1), 0);
    prop(OBJ_MTL_KD, Kd, vec3(1), 0);
    prop(OBJ_MTL_KS, Ks, vec3(1), 0);
    prop(OBJ_MTL_KE, Ke, vec3(1), 0);

    prop(OBJ_MTL_MAP_KA, map_Ka, TEX_UNLOADED, MAT_AMBIENT_TEX);
    prop(OBJ_MTL_MAP_KD, map_Kd, TEX_UNLOADED, MAT_DIFFUSE_TEX);
    prop(OBJ_MTL_MAP_KS, map_Ks, TEX_UNLOADED, MAT_SPECULAR_TEX);
    prop(OBJ_MTL_MAP_KE, map_Ke, TEX_UNLOADED, MAT_EMISSIVE_TEX);
    prop(OBJ_MTL_MAP_NS, map_Ns, TEX_UNLOADED, MAT_SHININESS_TEX);
    prop(OBJ_MTL_MAP_D, map_d, TEX_UNLOADED, MAT_TRANSPARENCY_TEX);
    prop(OBJ_MTL_MAP_BUMP, map_bump, TEX_UNLOADED, MAT_NORMAL_TANGENT_TEX);

    if (obj.flags & (OBJ_MTL_D | OBJ_MTL_MAP_D)) {
        mat.flags |= MAT_TRANSPARENCY;
    }

    #undef prop
}

void obj2mesh_meshPart(const OBJMeshPart &obj, MeshPart &part) {
    part.offset = obj.indexOffset;
    part.size = obj.indexSize;
    part.material = u16(obj.materialIndex);
}

void obj2mesh(OBJMesh &obj, Mesh &mesh) {
    mesh.parts.resize(obj.meshParts.size());
    mesh.materials.resize(obj.materials.size());
    mesh.textures.resize(obj.textures.size());
    mesh.size = obj.indices.size();

    for (int c = 0, n = obj.textures.size(); c < n; c++) {
        obj2mesh_texture(obj.textures[c], mesh.textures[c]);
    }

    for (int c = 0, n = obj.materials.size(); c < n; c++) {
        obj2mesh_material(obj.materials[c], mesh.materials[c]);
    }

    for (int c = 0, n = obj.meshParts.size(); c < n; c++) {
        obj2mesh_meshPart(obj.meshParts[c], mesh.parts[c]);
    }

    optimizeMesh(mesh.parts, obj.indices);

    for (int c = 0, n = mesh.parts.size(); c < n; c++) {
        mesh.parts[c].shader = findShader(mesh, mesh.materials[mesh.parts[c].material]);
    }

    mesh.vao = createVao();
    glBufferData(GL_ARRAY_BUFFER, obj.verts.size() * sizeof(obj.verts[0]), obj.verts.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(obj.indices[0]), obj.indices.data(), GL_STATIC_DRAW);
    checkError();
}
