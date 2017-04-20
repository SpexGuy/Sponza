//
// Created by Martin Wickham on 4/17/17.
//

#include <cstdlib>
#include "material.h"
#include "gl_includes.h"

using namespace glm;


// ------------------- Begin Shader Text ----------------------

const char *vert = GLSL(
        uniform mat3 normalMat;
        uniform mat4 mvp;

        // These constants are duplicated in material.h as VAO_*
        layout(location=0) in vec3 position;
        layout(location=1) in vec3 normal;
        layout(location=2) in vec2 tex;

        out vec3 f_normal;
        out vec2 f_tex;

        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            f_normal = normalMat * normal;
            f_tex = tex;
        }
);

const char *diffuseTexFrag = GLSL(
        const vec3 lightDir = normalize(vec3(1,1,1));

        uniform sampler2D ambientTex;
        uniform vec3 ambientFilter;
        uniform sampler2D diffuseTex;
        uniform vec3 diffuseFilter;

        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            vec3 ambientColor = texture(ambientTex, f_tex).rgb;
            vec3 diffuseColor = texture(diffuseTex, f_tex).rgb;
            float kDiffuse = max(0, dot(lightDir, normalize(f_normal)));
            vec3 color = ambientColor * ambientFilter +
                         diffuseColor * diffuseFilter * kDiffuse;
            fragColor = vec4(color, 1);
        }
);

const char *texCoordFrag = GLSL(
        const float ambient = 0.2;
        const vec3 lightDir = normalize(vec3(1,1,1));

        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            // lighting is not realistic. Meant just to distinguish the faces.
            vec2 tex = abs(f_tex);
            float blue2 = 1 - tex.x*tex.x - tex.y*tex.y;
            vec3 color = vec3(tex, sqrt(max(0, blue2)));
            float light = dot(normalize(f_normal), lightDir) / 2 + 0.5;
            light = light + ambient * (1 - light);
            fragColor = vec4(light * color, 1);
        }
);

const char *normalFrag = GLSL(
        const float ambient = 0.2;
        const vec3 lightDir = normalize(vec3(1,1,1));

        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            // lighting is not realistic. Meant just to distinguish the faces.
            vec3 normal = normalize(f_normal);
            vec3 color = abs(normal);
            float light = dot(normal, lightDir) / 2 + 0.5;
            light = light + ambient * (1 - light);
            fragColor = vec4(light * color, 1);
        }
);

// ------------------ End Shader Text -----------------------




// ------------------ Begin Shader Data ---------------------

Shader shaders[kNumShaders];
const Shader *currentShader = nullptr;

// ------------------ End Shader Data -----------------------




// ------------------ Begin Shader Uniforms -----------------

struct {
    GLuint normalMat;
    GLuint mvp;
} texCoordUniforms;

struct {
    GLuint normalMat;
    GLuint mvp;
    GLuint ambientTex;
    GLuint ambientFilter;
    GLuint diffuseTex;
    GLuint diffuseFilter;
} diffuseTexUniforms;


template<typename U>
inline void bindUniformsBase(const U &uniforms, const glm::mat4 &mvp, const glm::mat3 &normal) {
    glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix3fv(uniforms.normalMat, 1, GL_FALSE, &normal[0][0]);
}

void texCoordBindUniforms(const glm::mat4 &mvp, const glm::mat3 &normal, const OBJMesh &mesh, const OBJMaterial &material) {
    bindUniformsBase(texCoordUniforms, mvp, normal);
}

void diffuseTexBindUniforms(const glm::mat4 &mvp, const glm::mat3 &normal, const OBJMesh &mesh, const OBJMaterial &material) {
    bindUniformsBase(diffuseTexUniforms, mvp, normal);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_Ka].texName);
    glUniform1i(diffuseTexUniforms.ambientTex, 0);

    if (material.map_Ka == material.map_Kd) {
        glUniform1i(diffuseTexUniforms.diffuseTex, 0);
    } else {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_Kd].texName);
        glUniform1i(diffuseTexUniforms.diffuseTex, 1);
    }


    if (material.flags & OBJ_MTL_KA) {
        glUniform3f(diffuseTexUniforms.ambientFilter, material.Ka.r, material.Ka.g, material.Ka.b);
    } else {
        glUniform3f(diffuseTexUniforms.ambientFilter, 1,1,1);
    }

    if (material.flags & OBJ_MTL_KD) {
        glUniform3f(diffuseTexUniforms.diffuseFilter, material.Kd.r, material.Kd.g, material.Kd.b);
    } else {
        glUniform3f(diffuseTexUniforms.diffuseFilter, 1,1,1);
    }
}
// ------------------ End Shader Uniforms -------------------




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

inline bool all(OBJMaterialFlags flags, OBJMaterialFlags mask) {
    return (flags & mask) == mask;
}

// ------------------ End Utility Functions -----------------



void initShaders() {
    #define getUniform(storage, field) \
        do { storage.field = glGetUniformLocation(shader, #field); } while (0)

    GLuint shader = compileShader(vert, texCoordFrag);
    getUniform(texCoordUniforms, mvp);
    getUniform(texCoordUniforms, normalMat);
    shaders[kTexCoord].program = shader;
    shaders[kTexCoord].bindUniforms = texCoordBindUniforms;
    checkError();

    // The normal shader has the same uniforms as the texCoord shader.
    shader = compileShader(vert, normalFrag);
    shaders[kNormal].program = shader;
    shaders[kNormal].bindUniforms = texCoordBindUniforms;

    shader = compileShader(vert, diffuseTexFrag);
    getUniform(diffuseTexUniforms, mvp);
    getUniform(diffuseTexUniforms, normalMat);
    getUniform(diffuseTexUniforms, ambientTex);
    getUniform(diffuseTexUniforms, ambientFilter);
    getUniform(diffuseTexUniforms, diffuseTex);
    getUniform(diffuseTexUniforms, diffuseFilter);
    shaders[kDiffuseTex].program = shader;
    shaders[kDiffuseTex].bindUniforms = diffuseTexBindUniforms;
    checkError();

    bindShader(shaders[kTexCoord]);

    for (int c = 0; c < kNumShaders; c++) {
        u32 program = shaders[c].program;
        assert(glGetAttribLocation(program, "position") == VAO_POS);
        assert(glGetAttribLocation(program, "normal") == VAO_NOR);
        assert(glGetAttribLocation(program, "tex") == VAO_TEX);
    }

    #undef getUniform
}

Shader &getShader(int shader) {
    return shaders[shader];
}

bool validTexture(const OBJMesh &mesh, u32 texName) {
    return mesh.textures[texName].texName != UNLOADED;
}

Shader &findShader(const OBJMesh &mesh, const OBJMaterial &material) {
    bool ka = (material.flags & OBJ_MTL_MAP_KA) != 0;
    bool kd = (material.flags & OBJ_MTL_MAP_KD) != 0;
    bool bump = (material.flags & OBJ_MTL_MAP_BUMP) != 0;

    if (ka && !validTexture(mesh, material.map_Ka)) return shaders[kNormal];
    if (kd && !validTexture(mesh, material.map_Kd)) return shaders[kNormal];
    if (bump && !validTexture(mesh, material.map_bump)) return shaders[kNormal];

//    if (ka && kd && bump) return shaders[kBumpTex];
//    else if (bump) return shaders[kNormal];

    if (ka && kd) return shaders[kDiffuseTex];

    return shaders[kNormal];
}

void bindShader(const Shader &shader) {
    if (currentShader == &shader) return;
    glUseProgram(shader.program);
    currentShader = &shader;
    checkError();
}

void bindMaterial(const glm::mat4 &mvp, const glm::mat3 &normal, const OBJMesh &mesh, const OBJMaterial &material) {
    currentShader->bindUniforms(mvp, normal, mesh, material);
    checkError();
}
