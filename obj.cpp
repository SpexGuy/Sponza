//
// Created by Martin Wickham on 4/10/17.
//

#include "obj.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace glm;

void readVec(istream &input, vec3 &vec) {
    input >> vec.r >> vec.g >> vec.b;
}

bool loadMaterials(const string &filename, vector<OBJMaterial> &materials) {
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
            materials.emplace_back();
            current = &materials.back();
            lineReader >> current->name;
        } else {
            if (!current) {
                printf("Invalid material file: %s (first token is '%s' not 'newmtl'), line %d\n", filename.c_str(), token.c_str(), lineNum);
                return false;
            }

            // Parse token
            MaterialFlags flag = 0;
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
                lineReader >> current->map_Ka;
            } else if (token == "map_Kd") {
                flag = OBJ_MTL_MAP_KD;
                lineReader >> current->map_Kd;
            } else if (token == "map_Ks") {
                flag = OBJ_MTL_MAP_KS;
                lineReader >> current->map_Ks;
            } else if (token == "map_Ke") {
                flag = OBJ_MTL_MAP_KE;
                lineReader >> current->map_Ke;
            } else if (token == "map_Ns") {
                flag = OBJ_MTL_MAP_NS;
                lineReader >> current->map_Ns;
            } else if (token == "map_d") {
                flag = OBJ_MTL_MAP_D;
                lineReader >> current->map_d;
            } else if (token == "map_bump" || token == "bump") {
                if (current->flags & OBJ_MTL_MAP_BUMP) {
                    string tmp;
                    lineReader >> tmp;
                    if (tmp != current->map_bump) {
                        printf("Warning: multiple bump maps. (material file %s, line %d)\n", filename.c_str(), lineNum);
                    }
                } else {
                    flag = OBJ_MTL_MAP_BUMP;
                    lineReader >> current->map_bump;
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
