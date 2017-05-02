//
// Created by Martin Wickham on 4/17/17.
//

#include <cstdlib>
#include "material.h"
#include "gl_includes.h"

using namespace glm;


struct Shader {
    GLuint program;
    void (*bindUniforms)(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material);
};


// ------------------- Begin Shader Text ----------------------

const char *vert = GLSL(
        uniform mat4 mvp;

        // These constants are duplicated in material.h as VAO_*
        layout(location=0) in vec3 position;
        layout(location=1) in vec3 normal;
        layout(location=2) in vec2 tex;

        out vec3 f_position;
        out vec3 f_normal;
        out vec2 f_tex;

        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            f_position = position;
            f_normal = normal;
            f_tex = tex;
        }
);

const char *diffuseTexFrag = GLSL(
        uniform vec3 lightPos;
        uniform sampler2D ambientTex;
        uniform vec3 ambientFilter;
        uniform sampler2D diffuseTex;
        uniform vec3 diffuseFilter;

        in vec3 f_position;
        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            vec3 ambientColor = texture(ambientTex, f_tex).rgb;
            vec3 diffuseColor = texture(diffuseTex, f_tex).rgb;
            vec3 lightDir = normalize(lightPos - f_position);
            vec3 normal = normalize(f_normal);
            float kDiffuse = max(0, dot(lightDir, normal));
            vec3 color = ambientColor * ambientFilter +
                         diffuseColor * diffuseFilter * kDiffuse;
            fragColor = vec4(color, 1);
        }
);

const char *diffuseAlphaTexFrag = GLSL(
        uniform vec3 lightPos;
        uniform sampler2D ambientTex;
        uniform vec3 ambientFilter;
        uniform sampler2D diffuseTex;
        uniform vec3 diffuseFilter;
        uniform sampler2D alphaTex;

        in vec3 f_position;
        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            float alpha = texture(alphaTex, f_tex).r;
            if (alpha < 0.5) discard;

            vec3 ambientColor = texture(ambientTex, f_tex).rgb;
            vec3 diffuseColor = texture(diffuseTex, f_tex).rgb;
            vec3 lightDir = normalize(lightPos - f_position);
            vec3 normal = normalize(f_normal);
            float kDiffuse = max(0, dot(lightDir, normal));
            vec3 color = ambientColor * ambientFilter +
                         diffuseColor * diffuseFilter * kDiffuse;
            fragColor = vec4(color, 1);
        }
);

const char *specularTexFrag = GLSL(
        uniform vec3 lightPos;
        uniform vec3 camPosition;
        uniform sampler2D ambientTex;
        uniform vec3 ambientFilter;
        uniform sampler2D diffuseTex;
        uniform vec3 diffuseFilter;
        uniform sampler2D specularTex;
        uniform float shininess;

        in vec3 f_position;
        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            vec3 ambientColor = texture(ambientTex, f_tex).rgb;
            vec3 diffuseColor = texture(diffuseTex, f_tex).rgb;
            vec3 specularColor = texture(specularTex, f_tex).rgb;
            vec3 lightDir = normalize(lightPos - f_position);
            vec3 normal = normalize(f_normal);
            vec3 viewDir = normalize(camPosition-f_position);
            float kDiffuse = max(0, dot(lightDir, normal));
            vec3 halfway = normalize(lightDir + viewDir);
            float specAngle = max(0, dot(halfway, normal));
            float kSpecular = pow(specAngle, shininess);
            vec3 color = ambientColor * ambientFilter +
                         diffuseColor * diffuseFilter * kDiffuse +
                         specularColor * kSpecular;
            fragColor = vec4(color, 1);
        }
);

const char *specularAlphaTexFrag = GLSL(
        uniform vec3 lightPos;
        uniform vec3 camPosition;
        uniform sampler2D ambientTex;
        uniform vec3 ambientFilter;
        uniform sampler2D diffuseTex;
        uniform vec3 diffuseFilter;
        uniform sampler2D specularTex;
        uniform float shininess;
        uniform sampler2D alphaTex;

        in vec3 f_position;
        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            float alpha = texture(alphaTex, f_tex).r;
            if (alpha < 0.5) discard;

            vec3 ambientColor = texture(ambientTex, f_tex).rgb;
            vec3 diffuseColor = texture(diffuseTex, f_tex).rgb;
            vec3 specularColor = texture(specularTex, f_tex).rgb;
            vec3 lightDir = normalize(lightPos - f_position);
            vec3 normal = normalize(f_normal);
            vec3 viewDir = normalize(camPosition-f_position);
            float kDiffuse = max(0, dot(lightDir, normal));
            vec3 halfway = normalize(lightDir + viewDir);
            float specAngle = max(0, dot(halfway, normal));
            float kSpecular = pow(specAngle, shininess);
            vec3 color = ambientColor * ambientFilter +
                         diffuseColor * diffuseFilter * kDiffuse +
                         specularColor * kSpecular;
            fragColor = vec4(color, 1);
        }
);

const char *texCoordFrag = GLSL(
        const float ambient = 0.2;
        uniform vec3 lightPos;

        in vec3 f_position;
        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            // lighting is not realistic. Meant just to distinguish the faces.
            vec2 tex = abs(f_tex);
            float blue2 = 1 - tex.x*tex.x - tex.y*tex.y;
            vec3 color = vec3(tex, sqrt(max(0, blue2)));
            vec3 lightDir = normalize(lightPos - f_position);
            float light = dot(normalize(f_normal), lightDir) / 2 + 0.5;
            light = light + ambient * (1 - light);
            fragColor = vec4(light * color, 1);
        }
);

const char *normalFrag = GLSL(
        const float ambient = 0.2;
        uniform vec3 lightPos;

        in vec3 f_position;
        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            // lighting is not realistic. Meant just to distinguish the faces.
            vec3 normal = normalize(f_normal);
            vec3 color = abs(normal);
            vec3 lightDir = normalize(lightPos - f_position);
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
    GLuint mvp;
    GLuint lightPos;
} texCoordUniforms;

struct {
    GLuint mvp;
    GLuint lightPos;
    GLuint ambientTex;
    GLuint ambientFilter;
    GLuint diffuseTex;
    GLuint diffuseFilter;
} diffuseTexUniforms;

struct {
    GLuint mvp;
    GLuint lightPos;
    GLuint ambientTex;
    GLuint ambientFilter;
    GLuint diffuseTex;
    GLuint diffuseFilter;
    GLuint alphaTex;
} diffuseAlphaTexUniforms;

struct {
    GLuint mvp;
    GLuint lightPos;
    GLuint camPosition;
    GLuint ambientTex;
    GLuint ambientFilter;
    GLuint diffuseTex;
    GLuint diffuseFilter;
    GLuint specularTex;
    GLuint shininess;
} specularTexUniforms;

struct {
    GLuint mvp;
    GLuint lightPos;
    GLuint camPosition;
    GLuint ambientTex;
    GLuint ambientFilter;
    GLuint diffuseTex;
    GLuint diffuseFilter;
    GLuint specularTex;
    GLuint shininess;
    GLuint alphaTex;
} specularAlphaTexUniforms;


template<typename U>
inline void bindUniformsBase(const U &uniforms, const glm::mat4 &mvp, const glm::vec3 &lightPos) {
    glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, &mvp[0][0]);
    glUniform3f(uniforms.lightPos, lightPos.x, lightPos.y, lightPos.z);
}

template<typename U>
inline void bindUniformsDiffuse(const U &uniforms, const Mesh &mesh, const Material &material) {
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

template<typename U>
inline void bindUniformsSpecular(const U &uniforms, const vec3 &camPosition, const Mesh &mesh, const Material &material) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_Ks].glHandle);
    glUniform1i(uniforms.specularTex, 2);

    glUniform3f(uniforms.camPosition, camPosition.x, camPosition.y, camPosition.z);
    glUniform1f(uniforms.shininess, material.Ns);
}

template<typename U>
inline void bindUniformsAlpha(const U &uniforms, const Mesh &mesh, const Material &material) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mesh.textures[material.map_d].glHandle);
    glUniform1i(uniforms.alphaTex, 3);
}

void texCoordBindUniforms(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniformsBase(texCoordUniforms, mvp, lightPos);
}

void diffuseTexBindUniforms(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniformsBase(diffuseTexUniforms, mvp, lightPos);
    bindUniformsDiffuse(diffuseTexUniforms, mesh, material);
}

void specularTexBindUniforms(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniformsBase(specularTexUniforms, mvp, lightPos);
    bindUniformsDiffuse(specularTexUniforms, mesh, material);
    bindUniformsSpecular(specularTexUniforms, camPos, mesh, material);
}

void diffuseAlphaTexBindUniforms(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniformsBase(diffuseAlphaTexUniforms, mvp, lightPos);
    bindUniformsDiffuse(diffuseAlphaTexUniforms, mesh, material);
    bindUniformsAlpha(diffuseAlphaTexUniforms, mesh, material);
}

void specularAlphaTexBindUniforms(const glm::mat4 &mvp, const glm::vec3 &camPos, const glm::vec3 &lightPos, const Mesh &mesh, const Material &material) {
    bindUniformsBase(specularAlphaTexUniforms, mvp, lightPos);
    bindUniformsDiffuse(specularAlphaTexUniforms, mesh, material);
    bindUniformsSpecular(specularAlphaTexUniforms, camPos, mesh, material);
    bindUniformsAlpha(specularAlphaTexUniforms, mesh, material);
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

inline bool all(MaterialFlags flags, MaterialFlags mask) {
    return (flags & mask) == mask;
}

// ------------------ End Utility Functions -----------------



void initShaders() {
    #define getUniform(storage, field) \
        do { storage.field = glGetUniformLocation(shader, #field); } while (0)

    GLuint shader = compileShader(vert, texCoordFrag);
    getUniform(texCoordUniforms, mvp);
    getUniform(texCoordUniforms, lightPos);
    shaders[kTexCoord].program = shader;
    shaders[kTexCoord].bindUniforms = texCoordBindUniforms;
    checkError();

    // The normal shader has the same uniforms as the texCoord shader.
    shader = compileShader(vert, normalFrag);
    shaders[kNormal].program = shader;
    shaders[kNormal].bindUniforms = texCoordBindUniforms;

    shader = compileShader(vert, diffuseTexFrag);
    getUniform(diffuseTexUniforms, mvp);
    getUniform(diffuseTexUniforms, lightPos);
    getUniform(diffuseTexUniforms, ambientTex);
    getUniform(diffuseTexUniforms, ambientFilter);
    getUniform(diffuseTexUniforms, diffuseTex);
    getUniform(diffuseTexUniforms, diffuseFilter);
    shaders[kDiffuseTex].program = shader;
    shaders[kDiffuseTex].bindUniforms = diffuseTexBindUniforms;
    checkError();

    shader = compileShader(vert, specularTexFrag);
    getUniform(specularTexUniforms, mvp);
    getUniform(specularTexUniforms, lightPos);
    getUniform(specularTexUniforms, camPosition);
    getUniform(specularTexUniforms, ambientTex);
    getUniform(specularTexUniforms, ambientFilter);
    getUniform(specularTexUniforms, diffuseTex);
    getUniform(specularTexUniforms, diffuseFilter);
    getUniform(specularTexUniforms, specularTex);
    getUniform(specularTexUniforms, shininess);
    shaders[kSpecularTex].program = shader;
    shaders[kSpecularTex].bindUniforms = specularTexBindUniforms;
    checkError();

    shader = compileShader(vert, diffuseAlphaTexFrag);
    getUniform(diffuseAlphaTexUniforms, mvp);
    getUniform(diffuseAlphaTexUniforms, lightPos);
    getUniform(diffuseAlphaTexUniforms, ambientTex);
    getUniform(diffuseAlphaTexUniforms, ambientFilter);
    getUniform(diffuseAlphaTexUniforms, diffuseTex);
    getUniform(diffuseAlphaTexUniforms, diffuseFilter);
    getUniform(diffuseAlphaTexUniforms, alphaTex);
    shaders[kDiffuseAlphaTex].program = shader;
    shaders[kDiffuseAlphaTex].bindUniforms = diffuseAlphaTexBindUniforms;
    checkError();

    shader = compileShader(vert, specularAlphaTexFrag);
    getUniform(specularAlphaTexUniforms, mvp);
    getUniform(specularAlphaTexUniforms, lightPos);
    getUniform(specularAlphaTexUniforms, camPosition);
    getUniform(specularAlphaTexUniforms, ambientTex);
    getUniform(specularAlphaTexUniforms, ambientFilter);
    getUniform(specularAlphaTexUniforms, diffuseTex);
    getUniform(specularAlphaTexUniforms, diffuseFilter);
    getUniform(specularAlphaTexUniforms, specularTex);
    getUniform(specularAlphaTexUniforms, shininess);
    getUniform(specularAlphaTexUniforms, alphaTex);
    shaders[kSpecularAlphaTex].program = shader;
    shaders[kSpecularAlphaTex].bindUniforms = specularAlphaTexBindUniforms;
    checkError();

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
    currentShader->bindUniforms(mvp, camPos, lightPos, mesh, material);
    checkError();
}
