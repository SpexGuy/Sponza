//
// Created by Martin Wickham on 4/10/17.
//

#include "obj.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace glm;

void trimString(string &str) {
    const char * whiteSpace = " \t\n\r";
    size_t location;
    location = str.find_first_not_of(whiteSpace);
    str.erase(0, location);
    location = str.find_last_not_of(whiteSpace);
    str.erase(location + 1);
}

u32 findMaterialIndex(vector<OBJMaterial> materials, string name) {
    for (u32 c = 0, n = materials.size(); c < n; c++) {
        if (materials[c].name == name) {
            return c;
        }
    }
    printf("Warning: Unknown material: %s\n", name.c_str());
    return 0;
}


void readVec(istream &input, vec3 &vec) {
    input >> vec.r >> vec.g >> vec.b;
}

void readTex(OBJMesh &mesh, istream &input, u16 &tex) {
    string texname;
    input >> texname;
    for (u32 c = 0, n = mesh.textures.size(); c < n; c++) {
        if (mesh.textures[c].name == texname) {
            tex = u16(c);
            return;
        }
    }

    mesh.textures.emplace_back();
    mesh.textures.back().name = texname;
    tex = u16(mesh.textures.size() - 1);
}

bool loadMaterials(const string &filename, OBJMesh &mesh) {
    ifstream input(filename);
    if (!input) {
        printf("Failed to open material file: %s\n", filename.c_str());
        return false;
    }

    OBJMaterial *current = nullptr;

    u32 lineNum = 0;
    string line, token;
    while (getline(input, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        istringstream lineReader(line);
        lineReader >> token;
        if (!lineReader || token.empty() || token[0] == '#') {
            continue;
        }

        if (token == "newmtl") {
            mesh.materials.emplace_back();
            current = &mesh.materials.back();
            lineReader >> current->name;
        } else {
            if (!current) {
                printf("Invalid material file: %s (first token is '%s' not 'newmtl'), line %d\n", filename.c_str(), token.c_str(), lineNum);
                return false;
            }

            // Parse token
            OBJMaterialFlags flag = 0;
            if (token == "Ns") {
                flag = OBJ_MTL_NS;
                lineReader >> current->Ns;
            } else if (token == "Ni") {
                flag = OBJ_MTL_NI;
                lineReader >> current->Ni;
            } else if (token == "d") {
                float d = 1;
                lineReader >> d;
                if (current->flags & OBJ_MTL_D) {
                    if (current->d != d) {
                        printf("Warning: Multiple transparency values (material file %s, line %d)\n", filename.c_str(), lineNum);
                    }
                }
                current->flags |= OBJ_MTL_D;
                current->d = d;
            } else if (token == "Tr") {
                float Tr = 0;
                lineReader >> Tr;
                float d = 1-Tr;
                if (current->flags & OBJ_MTL_D) {
                    if (current->d != d) {
                        printf("Warning: Multiple transparency values (material file %s, line %d)\n", filename.c_str(), lineNum);
                    }
                }
                current->flags |= OBJ_MTL_D;
                current->d = d;
            } else if (token == "Tf") {
                flag = OBJ_MTL_TF;
                readVec(lineReader, current->Tf);
            } else if (token == "illum") {
                flag = OBJ_MTL_ILLUM;
                lineReader >> current->illum;
            } else if (token == "Ka") {
                flag = OBJ_MTL_KA;
                readVec(lineReader, current->Ka);
            } else if (token == "Kd") {
                flag = OBJ_MTL_KD;
                readVec(lineReader, current->Kd);
            } else if (token == "Ks") {
                flag = OBJ_MTL_KS;
                readVec(lineReader, current->Ks);
            } else if (token == "Ke") {
                flag = OBJ_MTL_KE;
                readVec(lineReader, current->Ke);
            } else if (token == "map_Ka") {
                flag = OBJ_MTL_MAP_KA;
                readTex(mesh, lineReader, current->map_Ka);
            } else if (token == "map_Kd") {
                flag = OBJ_MTL_MAP_KD;
                readTex(mesh, lineReader, current->map_Kd);
            } else if (token == "map_Ks") {
                flag = OBJ_MTL_MAP_KS;
                readTex(mesh, lineReader, current->map_Ks);
            } else if (token == "map_Ke") {
                flag = OBJ_MTL_MAP_KE;
                readTex(mesh, lineReader, current->map_Ke);
            } else if (token == "map_Ns") {
                flag = OBJ_MTL_MAP_NS;
                readTex(mesh, lineReader, current->map_Ns);
            } else if (token == "map_d") {
                flag = OBJ_MTL_MAP_D;
                readTex(mesh, lineReader, current->map_d);
            } else if (token == "map_bump" || token == "bump") {
                if (current->flags & OBJ_MTL_MAP_BUMP) {
                    u16 tmp;
                    readTex(mesh, lineReader, tmp);
                    if (tmp != current->map_bump) {
                        printf("Warning: multiple bump maps. (material file %s, line %d)\n", filename.c_str(), lineNum);
                        current->map_bump = tmp;
                    }
                } else {
                    flag = OBJ_MTL_MAP_BUMP;
                    readTex(mesh, lineReader, current->map_bump);
                }
            } else {
                printf("Unknown directive: %s (in file %s, line %d)\n", token.c_str(), filename.c_str(), lineNum);
                continue;
            }

            // Validate
            if (!lineReader) {
                printf("Warning: Not enough tokens (material file %s, line %d)\n", filename.c_str(), lineNum);
            }
            if (flag & current->flags) {
                printf("Warning: Duplicate %s declaration (material file %s, line %d)\n", token.c_str(), filename.c_str(), lineNum);
            }
            current->flags |= flag;
        }
    }
    return true;
}

bool loadObjFile(const string &cwd, const string &filename, OBJMesh &mesh) {
    string pathname = cwd + '/' + filename;
    ifstream objStream(pathname);
    if (!objStream) {
        printf("Could not open obj file: %s\n", pathname.c_str());
        return false;
    }

    clock_t time = clock();
    vector<vec3> points;
    vector<vec3> normals;
    vector<vec2> texCoords;

    bool loadTex = true;

    string line, token;
    vector<ivec3> face;

    // init the first mesh part
    mesh.meshParts.push_back(OBJMeshPart { 0, u32(mesh.indices.size()), 0 });
    OBJMeshPart *current = &mesh.meshParts.back();
    string currentMtl;

    getline(objStream, line);
    while (!objStream.eof()) {
        trimString(line);
        if (line.length() > 0 && line.at(0) != '#') {
            istringstream lineStream(line);

            lineStream >> token;

            if (token == "mtllib") {
                string matfile;
                lineStream >> matfile;
                matfile = cwd + '/' + matfile;
                printf("Loading material file: %s\n", matfile.c_str());
                bool success = loadMaterials(matfile, mesh);
                if (!success) {
                    printf("Warning: failed to load material library %s\n", matfile.c_str());
                }
            } else if (token == "g") {
//                string group;
//                lineStream >> group;
//                printf("Starting group %s\n", group.c_str());
            } else if (token == "usemtl") {
                string mtl;
                lineStream >> mtl;
                if (mtl != currentMtl) {
                    // new mesh part
                    u32 indexCount = u32(mesh.indices.size());
                    if (indexCount != current->indexOffset) {
                        current->indexSize = indexCount - current->indexOffset;
                        mesh.meshParts.push_back(OBJMeshPart {0, indexCount, 0});
                        current = &mesh.meshParts.back();
                    }
                    current->materialIndex = findMaterialIndex(mesh.materials, mtl);
//                    printf("Switching material: %s\n", mtl.c_str());
                    currentMtl = std::move(mtl);
                }
            } else if (token == "v") {
                f32 x, y, z;
                lineStream >> x >> y >> z;
                points.push_back(vec3(x, y, z));
            } else if (token == "vt" && loadTex) {
                // Process texture coordinate
                f32 s, t;
                lineStream >> s >> t;
                t = 1 - t; // t is flipped in OBJ
                texCoords.push_back(vec2(s, t));
            } else if (token == "vn") {
                f32 x, y, z;
                lineStream >> x >> y >> z;
                normals.push_back(vec3(x, y, z));
            } else if (token == "f") {
                // Process face
                face.clear();
                size_t slash1, slash2;
                //int point, texCoord, normal;
                while (lineStream.good()) {
                    string vertString;
                    lineStream >> vertString;
                    int pIndex = -1, nIndex = -1, tcIndex = -1;

                    slash1 = vertString.find("/");
                    if (slash1 == string::npos){
                        pIndex = atoi(vertString.c_str()) - 1;
                    } else {
                        slash2 = vertString.find("/", slash1 + 1);
                        pIndex = atoi(vertString.substr(0, slash1).c_str()) - 1;
                        if (slash2 > slash1 + 1) {
                            tcIndex = atoi(vertString.substr(slash1 + 1, slash2).c_str()) - 1;
                        }
                        nIndex = atoi(vertString.substr(slash2 + 1, vertString.length()).c_str()) - 1;
                    }
                    if (pIndex == -1) {
                        printf("Missing point index!!!");
                    } else if (tcIndex == -1) {
                        printf("Missing texture index!!!");
                    } else if (nIndex == -1) {
                        printf("Missing normal index!!!");
                    } else {
                        face.push_back(ivec3(pIndex, tcIndex, nIndex));
                    }
                }
                ivec3 v0 = face[0];
                ivec3 v1 = face[1];
                ivec3 v2 = face[2];
                u32 baseIndex = mesh.verts.size();
                // First face
                mesh.indices.push_back(baseIndex);
                mesh.verts.push_back(OBJVertex { points[v0.x], normals[v0.z], texCoords[v0.y] });
                mesh.indices.push_back(mesh.verts.size());
                mesh.verts.push_back(OBJVertex { points[v1.x], normals[v1.z], texCoords[v1.y] });
                mesh.indices.push_back(mesh.verts.size());
                mesh.verts.push_back(OBJVertex { points[v2.x], normals[v2.z], texCoords[v2.y] });
                // If number of edges in face is greater than 3,
                // decompose into triangles as a triangle fan.
                for (u32 i = 3; i < face.size(); i++) {
                    v1 = v2;
                    v2 = face[i];
                    mesh.indices.push_back(baseIndex);
                    mesh.indices.push_back(mesh.verts.size()-1);
                    mesh.indices.push_back(mesh.verts.size());
                    mesh.verts.push_back(OBJVertex { points[v2.x], normals[v2.z], texCoords[v2.y] });
                }
            } else if (token != "s") {
                printf("Unknown token: %s\n", token.c_str());
            }
        }
        getline(objStream, line);
    }
    current->indexSize = mesh.indices.size() - current->indexOffset;

    printf("Loaded mesh from %s in %lums\n", filename.c_str(), (clock() - time) * 1000 / CLOCKS_PER_SEC);

    return true;
}
