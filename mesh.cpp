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
    glEnableVertexAttribArray(VAO_TAN);
    glEnableVertexAttribArray(VAO_BTN);
    glEnableVertexAttribArray(VAO_TEX);
    glVertexAttribPointer(VAO_POS, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(VAO_NOR, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec3));
    glVertexAttribPointer(VAO_TAN, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(vec3) * 2));
    glVertexAttribPointer(VAO_BTN, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(vec3) * 3));
    glVertexAttribPointer(VAO_TEX, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(vec3) * 4));
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
    prop(OBJ_MTL_KS, Ks, vec3(0), 0);
    prop(OBJ_MTL_KE, Ke, vec3(0), 0);

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

inline float cross(vec2 a, vec2 b) {
    return a.x * b.y - b.x * a.y;
}

void bufferObjVertexData(const OBJMesh &obj) {
    vector<Vertex> updatedVerts;
    updatedVerts.resize(obj.verts.size());
    for (u32 c = 0, n = obj.verts.size(); c < n; c++) {
        const OBJVertex &overt = obj.verts[c];
        Vertex &vert = updatedVerts[c];
        vert.position = overt.position;
        vert.normal = overt.normal;
        vert.tex = overt.texture;
    }

    for (u32 c = 0, n = obj.indices.size(); c < n; c += 3) {
        int idx0 = obj.indices[c+0];
        int idx1 = obj.indices[c+1];
        int idx2 = obj.indices[c+2];
        Vertex &v0 = updatedVerts[idx0];
        Vertex &v1 = updatedVerts[idx1];
        Vertex &v2 = updatedVerts[idx2];

        vec3 p1 = v1.position - v0.position;
        vec3 p2 = v2.position - v0.position;

        vec3 normal = cross(p1, p2);
        if (normal == vec3(0)) continue;
        else normal = normalize(normal);
        assert(!any(isnan(normal)));

        vec2 t1 = v1.tex - v0.tex;
        vec2 t2 = v2.tex - v0.tex;

        float r = 1.f / cross(t1, t2);
        if (glm::isinf(r)) continue;

        vec3 pu = (t2.y * p1 - t1.y * p2) * r;
        vec3 pv = (t1.x * p2 - t2.x * p1) * r;
        assert(!glm::isnan(r));
        assert(!any(isnan(pu)));
        assert(!any(isnan(pv)));

        vec3 tan = cross(pv, normal);
        vec3 btn = cross(normal, pu);
        assert(!any(isnan(tan)));
        assert(!any(isnan(btn)));

        v0.tangent += tan;
        v1.tangent += tan;
        v2.tangent += tan;
        v0.bitangent += btn;
        v1.bitangent += btn;
        v2.bitangent += btn;
    }

    for (u32 c = 0, n = updatedVerts.size(); c < n; c++) {
        Vertex &vert = updatedVerts[c];
        if (vert.bitangent != vec3(0) || vert.tangent != vec3(0)) {
            if (vert.bitangent == vec3(0)) {
                vert.tangent = normalize(vert.tangent);
                vert.bitangent = normalize(cross(vert.normal, vert.tangent));
            } else if (vert.tangent == vec3(0)) {
                vert.bitangent = normalize(vert.bitangent);
                vert.tangent = normalize(cross(vert.bitangent, vert.normal));
            } else {
                vert.tangent = normalize(vert.tangent);
                vert.bitangent = normalize(vert.bitangent);
            }
        }
    }

    glBufferData(GL_ARRAY_BUFFER, updatedVerts.size() * sizeof(updatedVerts[0]), updatedVerts.data(), GL_STATIC_DRAW);
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
    bufferObjVertexData(obj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(obj.indices[0]), obj.indices.data(), GL_STATIC_DRAW);
    checkError();
}
