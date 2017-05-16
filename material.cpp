//
// Created by Martin Wickham on 4/17/17.
//

#include <fstream>
#include <cstdlib>
#include "material.h"
#include "gl_includes.h"

using namespace glm;


// ------------------- Begin Shader Text ----------------------

const char *vert = GLSL(
        uniform mat4 mvp;

        // These constants are duplicated in material.h as VAO_*
        layout(location=0) in vec3 position;
        layout(location=1) in vec3 normal;
        layout(location=2) in vec2 tex;
        layout(location=3) in vec3 tangent;
        layout(location=4) in vec3 bitangent;

        out vec3 f_position;
        out vec3 f_normal;
        out vec2 f_tex;
        out vec3 f_tangent;
        out vec3 f_bitangent;

        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            f_position = position;
            f_normal = normal;
            f_tex = tex;
            f_tangent = tangent;
            f_bitangent = bitangent;
        }
);

// ------------------ End Shader Text -----------------------



// ------------------ Begin Utility Functions ---------------

void checkShaderError(GLuint shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) return;

    printf("Shader Compile Failed.\n");

    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize == 0) {
        printf("No log found.\n");
        return;
    }

    GLchar *log = new GLchar[logSize];

    glGetShaderInfoLog(shader, logSize, &logSize, log);
    printf("%s\n", log);

    delete[] log;
}

void checkLinkError(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success) return;

    printf("Shader link failed.\n");

    GLint logSize = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize == 0) {
        printf("No log found.\n");
        return;
    }

    GLchar *log = new GLchar[logSize];

    glGetProgramInfoLog(program, logSize, &logSize, log);
    printf("%s\n", log);

    delete[] log;
}

GLuint compileShader(const char *vertSrc, const char *fragSrc) {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertSrc, nullptr);
    glCompileShader(vertex);
    checkShaderError(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragSrc, nullptr);
    glCompileShader(fragment);
    checkShaderError(fragment);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vertex);
    glAttachShader(shader, fragment);
    glLinkProgram(shader);
    checkLinkError(shader);

    return shader;
}

// ------------------ End Utility Functions -----------------



// ------------------ Begin Shader Data ---------------------

enum {
    AMBIENT_TEX,
    DIFFUSE_TEX,
    SPECULAR_TEX,
    TRANSPARENCY_TEX,
    NORMAL_TANGENT_TEX,
    NORMAL_COLOR,
    TEX_COORD_COLOR,

    kNumFlags
};

#define fAmbientTex (1<<AMBIENT_TEX)
#define fDiffuseTex (1<<DIFFUSE_TEX)
#define fSpecularTex (1<<SPECULAR_TEX)
#define fTransparencyTex (1<<TRANSPARENCY_TEX)
#define fNormalTangentTex (1<<NORMAL_TANGENT_TEX)
#define fNormalColor (1<<NORMAL_COLOR)
#define fTexCoordColor (1<<TEX_COORD_COLOR)

const char *flagNames[kNumFlags] = {
        "AMBIENT_TEX",
        "DIFFUSE_TEX",
        "SPECULAR_TEX",
        "TRANSPARENCY_TEX",
        "NORMAL_TANGENT_TEX",
        "NORMAL_COLOR",
        "TEX_COORD_COLOR"
};

#define MAX_SHADER_SIZE 4096
#define MAX_IDENT_SIZE 128

struct {
    char buffer[MAX_SHADER_SIZE];
    u16 offsets[kNumFlags];
} shader;

void loadMegashader() {
    memset(&shader, 0, sizeof(shader)); // just in case

    std::ifstream file("assets/shader.glsl", std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (!file) {
        printf("Error: Failed to open shader from assets/shader.glsl.");
        exit(1);
    }

    if (size >= MAX_SHADER_SIZE - 1) {
        printf("Error: Shader is too big (found %ld bytes, max is %d).", size, MAX_SHADER_SIZE-1);
        exit(1);
    }

    if (!file.read(shader.buffer, size)) {
        printf("Error: Failed to read shader.");
        exit(1);
    }

    file.close();

    // turn the buffer into a proper string
    shader.buffer[size] = '\0';

    // now find the "//efine" tags
    const char *key = "//efine ";
    const u16 len = u16(strlen(key));
    const u16 safeEnd = u16(size - len);
    char identifier[MAX_IDENT_SIZE];
    for (u16 c = 0; c < safeEnd; c++) {
        if (strncmp(&shader.buffer[c], key, len) == 0) {
            // we found a tag, now let's read it.
            u16 d;
            for (d = 0; d < MAX_IDENT_SIZE-1; d++) {
                char value = identifier[d] = shader.buffer[len+c+d];
                if (value == ' ' || value == '\n') break;
            }
            identifier[d] = '\0';
            // printf("Found tag '%s' at offset %d.\n", identifier, c);

            bool known = false;
            for (u8 ni = 0; ni < kNumFlags; ni++) {
                if (strncmp(flagNames[ni], identifier, MAX_IDENT_SIZE-1) == 0) {
                    shader.offsets[ni] = c;
                    known = true;
                    break;
                }
            }
            if (!known) {
                printf("Warning: Ignoring unknown definition: %s.\n", identifier);
            }
            c = c + len + d;
        }
    }

    for (int c = 0; c < kNumFlags; c++) {
        if (shader.offsets[c] == 0) {
            printf("Warning: no definition for %s\n", flagNames[c]);
        }
    }
}

inline GLuint compileMegashader() {
    return compileShader(vert, shader.buffer);
}

inline void enable(u8 flag) {
    u16 offset = shader.offsets[flag];
    if (offset == 0) return;
    shader.buffer[offset+0] = '#';
    shader.buffer[offset+1] = 'd';
}

inline void disable(u8 flag) {
    u16 offset = shader.offsets[flag];
    if (offset == 0) return;
    shader.buffer[offset+0] = '/';
    shader.buffer[offset+1] = '/';
}

inline void disableAll() {
    for (int c = 0; c < kNumFlags; c++) {
        disable(c);
    }
}

struct CommonUniforms {
    GLuint mvp;
    GLuint lightPos;
};

struct DiffuseUniforms {
    GLuint ambientTex;
    GLuint ambientFilter;
    GLuint diffuseTex;
    GLuint diffuseFilter;
};

struct SpecularUniforms {
    GLuint specularTex;
    GLuint shininess;
    GLuint camPosition;
};

struct MaskUniforms {
    GLuint alphaTex;
};

struct BumpUniforms {
    GLuint normalTex;
};

struct Uniforms {
    u32 flags;
    CommonUniforms common;
    DiffuseUniforms diffuse;
    SpecularUniforms specular;
    MaskUniforms mask;
    BumpUniforms bump;
};

struct Shader {
    GLuint program;
    Uniforms uniforms;
};

Shader shaders[kNumShaders];
const Shader *currentShader = nullptr;

// ------------------ End Shader Data -----------------------



// ------------------ Begin Shader Uniforms -----------------

inline Uniforms buildUniforms(GLuint shader, u32 flags) {
    Uniforms uniforms;
    uniforms.flags = flags;

    #define getUniform(storage, field) \
        do { uniforms.storage.field = glGetUniformLocation(shader, #field); } while (0)

    getUniform(common, mvp);
    getUniform(common, lightPos);

    if (flags & fDiffuseTex) {
        getUniform(diffuse, ambientTex);
        getUniform(diffuse, ambientFilter);
        getUniform(diffuse, diffuseTex);
        getUniform(diffuse, diffuseFilter);
    }
    if (flags & fSpecularTex) {
        getUniform(specular, specularTex);
        getUniform(specular, shininess);
        getUniform(specular, camPosition);
    }
    if (flags & fTransparencyTex) {
        getUniform(mask, alphaTex);
    }
    if (flags & fNormalTangentTex) {
        getUniform(bump, normalTex);
    }

    return uniforms;
}

inline void bindUniformsBase(const CommonUniforms &uniforms, const glm::mat4 &mvp, const glm::vec3 &lightPos) {
    glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, &mvp[0][0]);
    glUniform3f(uniforms.lightPos, lightPos.x, lightPos.y, lightPos.z);
}

inline void bindUniformsDiffuse(const DiffuseUniforms &uniforms, const Mesh &mesh, const Material &material) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_Ka].glHandle);
    glUniform1i(uniforms.ambientTex, 0);

    if (material.map_Ka == material.map_Kd) {
        glUniform1i(uniforms.diffuseTex, 0);
    } else {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_Kd].glHandle);
        glUniform1i(uniforms.diffuseTex, 1);
    }

    glUniform3f(uniforms.ambientFilter, material.Ka.r, material.Ka.g, material.Ka.b);
    glUniform3f(uniforms.diffuseFilter, material.Kd.r, material.Kd.g, material.Kd.b);
}

inline void bindUniformsSpecular(const SpecularUniforms &uniforms, const vec3 &camPosition, const Mesh &mesh, const Material &material) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_Ks].glHandle);
    glUniform1i(uniforms.specularTex, 2);

    glUniform3f(uniforms.camPosition, camPosition.x, camPosition.y, camPosition.z);
    glUniform1f(uniforms.shininess, material.Ns);
}

inline void bindUniformsAlpha(const MaskUniforms &uniforms, const Mesh &mesh, const Material &material) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_d].glHandle);
    glUniform1i(uniforms.alphaTex, 3);
}

inline void bindUniformsBump(const BumpUniforms &uniforms, const Mesh &mesh, const Material &material) {
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_bump].glHandle);
    glUniform1i(uniforms.normalTex, 4);
}

void bindUniforms(const Uniforms &uniforms, const glm::mat4 &mvp, const glm::vec3 camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniformsBase(uniforms.common, mvp, lightPos);

    if (uniforms.flags & fDiffuseTex) {
        bindUniformsDiffuse(uniforms.diffuse, mesh, material);
    }
    if (uniforms.flags & fSpecularTex) {
        bindUniformsSpecular(uniforms.specular, camPos, mesh, material);
    }
    if (uniforms.flags & fTransparencyTex) {
        bindUniformsAlpha(uniforms.mask, mesh, material);
    }
    if (uniforms.flags & fNormalTangentTex) {
        bindUniformsBump(uniforms.bump, mesh, material);
    }
}

// ------------------ End Shader Uniforms -------------------



void initShader(u16 handle, u16 flags) {
    int attr = 0;
    int tmp = flags;
    while (tmp) {
        if (tmp & 1) {
            enable(attr);
        }
        attr++;
        tmp >>= 1;
    }

    GLuint shader = compileMegashader();
    shaders[handle].program = shader;
    shaders[handle].uniforms = buildUniforms(shader, flags);

    disableAll();
    checkError();
}

void initShaders() {
    loadMegashader();

    initShader(kTexCoord, fTexCoordColor);
    initShader(kNormal, fNormalColor);
//    initShader(kBumpTex, fNormalColor | fNormalTangentTex);
    initShader(kDiffuseTex, fAmbientTex | fDiffuseTex);
    initShader(kSpecularTex, fAmbientTex | fDiffuseTex | fSpecularTex);
    initShader(kDiffuseAlphaTex, fAmbientTex | fDiffuseTex | fTransparencyTex);
    initShader(kSpecularAlphaTex, fAmbientTex | fDiffuseTex | fSpecularTex | fTransparencyTex);
//    initShader(kDiffuseBumpTex, fAmbientTex | fDiffuseTex | fNormalTangentTex);
//    initShader(kSpecularBumpTex, fAmbientTex | fDiffuseTex | fSpecularTex | fNormalTangentTex);
//    initShader(kDiffuseAlphaBumpTex, fAmbientTex | fDiffuseTex | fTransparencyTex | fNormalTangentTex);
//    initShader(kSpecularAlphaBumpTex, fAmbientTex | fDiffuseTex | fSpecularTex | fTransparencyTex | fNormalTangentTex);

    bindShader(kTexCoord);

    for (int c = 0; c < kNumShaders; c++) {
        u32 program = shaders[c].program;
        assert(glGetAttribLocation(program, "position") == VAO_POS);
        assert(glGetAttribLocation(program, "normal") == VAO_NOR);
        assert(glGetAttribLocation(program, "tex") == VAO_TEX);
    }

    #undef getUniform
}

bool validTexture(const Mesh &mesh, u32 texName) {
    return mesh.textures[texName].glHandle != BAD_TEX;
}

u16 findShader(const Mesh &mesh, const Material &material) {
    bool ka = (material.flags & MAT_AMBIENT_TEX) != 0;
    bool kd = (material.flags & MAT_DIFFUSE_TEX) != 0;
    bool ks = (material.flags & MAT_SPECULAR_TEX) != 0;
    bool d  = (material.flags & MAT_TRANSPARENCY_TEX) != 0;

    if (ka && !validTexture(mesh, material.map_Ka)) return kNormal;
    if (kd && !validTexture(mesh, material.map_Kd)) return kNormal;
    if (ks && !validTexture(mesh, material.map_Ks)) return kNormal;
    if (d && !validTexture(mesh, material.map_d)) return kNormal;

    if (ka && kd && ks && d) return kSpecularAlphaTex;
    else if (ka && kd && d) return kDiffuseAlphaTex;
    else if (d) return kNormal; // error

    if (ka && kd && ks) return kSpecularTex;
    else if (ks) return kNormal; // error

    if (ka && kd) return kDiffuseTex;

    return kNormal;
}

void bindShader(u16 handle) {
    Shader *shader = &shaders[handle];
    if (currentShader == shader) return;
    glUseProgram(shader->program);
    currentShader = shader;
    checkError();
}

void bindMaterial(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniforms(currentShader->uniforms, mvp, camPos, lightPos, mesh, material);
    checkError();
}
