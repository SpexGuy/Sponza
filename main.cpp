#include <iostream>
#include <cstdlib>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <stb/stb_image.h>
#include "types.h"
#include "gl_includes.h"
#include "Perf.h"
#include "obj.h"

using namespace std;
using namespace glm;

GLuint compileShader(const char *vertSrc, const char *fragSrc);
int loadTexture(GLuint texname, const char *filename, int format = STBI_default);

GLFWwindow *window;


const char *vert = GLSL(

        uniform mat3 normalMat;
        uniform mat4 mvp;

        in vec3 position;
        in vec3 normal;
        in vec2 tex;

        out vec3 f_normal;
        out vec2 f_tex;

        void main() {
            // lighting is not realistic. Meant just to distinguish the faces.
            gl_Position = mvp * vec4(position, 1.0);
            f_normal = normalMat * normal;
            f_tex = tex;
        }
);

const char *frag = GLSL(
        const float ambient = 0.2;
        const vec3 lightDir = normalize(vec3(1,1,1));

        in vec3 f_normal;
        in vec2 f_tex;

        out vec4 fragColor;

        void main() {
            vec2 tex = abs(f_tex);
            float blue2 = 1 - tex.x*tex.x - tex.y*tex.y;
            vec3 color = vec3(tex, sqrt(max(0, blue2)));
            float light = dot(normalize(f_normal), lightDir) / 2 + 0.5;
            light = light + ambient * (1 - light);
            fragColor = vec4(light * color, 1);
        }
);

struct {
    GLuint normalMat;
    GLuint mvp;
} uniforms;

mat4 rotation; // identity
mat4 view;
mat4 projection;

OBJMesh mesh;

int part = -1;

void optimizeMesh() {
    if (mesh.meshParts.size() == 0)
        return; // shouldn't happen but JIC

    stable_sort(mesh.meshParts.begin(), mesh.meshParts.end(),
                [](const OBJMeshPart &a, const OBJMeshPart &b) {
        return a.materialIndex < b.materialIndex;
    });

    vector<u32> newIndices(mesh.indices.size());
    vector<OBJMeshPart> newMeshParts;
    u32 currentMaterial = mesh.meshParts[0].materialIndex;
    u32 pos = 0;
    newMeshParts.push_back(OBJMeshPart {currentMaterial, pos, 0});
    for (OBJMeshPart &part : mesh.meshParts) {
        u32 mat = part.materialIndex;
        if (mat != currentMaterial) {
            newMeshParts.back().indexSize = pos - newMeshParts.back().indexOffset;
            currentMaterial = mat;
            newMeshParts.push_back(OBJMeshPart {currentMaterial, pos, 0});
        }
        memcpy(&newIndices[pos], &mesh.indices[part.indexOffset], part.indexSize * sizeof(u32));
        pos += part.indexSize;
    }
    assert(pos == mesh.indices.size());
    newMeshParts.back().indexSize = pos - newMeshParts.back().indexOffset;

    printf("Mesh optimized; number of parts reduced from %lu to %lu\n", mesh.meshParts.size(), newMeshParts.size());

    mesh.meshParts = std::move(newMeshParts);
    mesh.indices = std::move(newIndices);
}

void loadTextures(string dir) {
    for (OBJTexture &tex : mesh.textures) {
        if (tex.texName == UNLOADED) {
            string file = dir + '/' + tex.name;
            glGenTextures(1, &tex.texName);
            loadTexture(tex.texName, file.c_str(), STBI_rgb);
        }
    }
}

void setup() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    bool success = loadObjFile("assets/sponza", "sponza.obj", mesh);
    if (!success) {
        printf("Failed to load materials.\n");
        exit(2);
    }
    printf("Loaded %lu materials.\n", mesh.materials.size());
    printf("Loaded %lu mesh parts.\n", mesh.meshParts.size());
    printf("Loaded %lu vertices and %lu indices.\n", mesh.verts.size(), mesh.indices.size());

    optimizeMesh();

    loadTextures("assets/sponza");

    GLuint shader = compileShader(vert, frag);
    glUseProgram(shader);
    uniforms.mvp = glGetUniformLocation(shader, "mvp");
    uniforms.normalMat = glGetUniformLocation(shader, "normalMat");
    checkError();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint verts, indices;
    glGenBuffers(1, &verts);
    glGenBuffers(1, &indices);
    glBindBuffer(GL_ARRAY_BUFFER, verts);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    glBufferData(GL_ARRAY_BUFFER, mesh.verts.size() * sizeof(mesh.verts[0]), mesh.verts.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(mesh.indices[0]), mesh.indices.data(), GL_STATIC_DRAW);

    GLuint pos = glGetAttribLocation(shader, "position");
    GLuint nor = glGetAttribLocation(shader, "normal");
    GLuint tex = glGetAttribLocation(shader, "tex");
    glEnableVertexAttribArray(pos);
    glEnableVertexAttribArray(nor);
    glEnableVertexAttribArray(tex);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(OBJVertex), 0);
    glVertexAttribPointer(nor, 3, GL_FLOAT, GL_FALSE, sizeof(OBJVertex), (void *) sizeof(vec3));
    glVertexAttribPointer(tex, 2, GL_FLOAT, GL_FALSE, sizeof(OBJVertex), (void *) (sizeof(vec3) * 2));
    checkError();

    view = lookAt(vec3(0, 0, 7000), vec3(0), vec3(0, 1, 0));
    rotation = lookAt(vec3(0), vec3(-1), vec3(0, 1, 0));
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // screw mesh parts, just draw everything.
    if (part == -1) {
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        OBJMeshPart &mp = mesh.meshParts[part];
        glDrawElements(GL_TRIANGLES, mp.indexSize, GL_UNSIGNED_INT, (void *)(mp.indexOffset * sizeof(u32)));
    }
}

void updateMatrices() {
    mat4 mvp = projection * view * rotation;
    checkError();
    glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, &mvp[0][0]);
    mat3 normal = mat3(view * rotation);
    glUniformMatrix3fv(uniforms.normalMat, 1, GL_FALSE, &normal[0][0]);
}

static void glfw_resize_callback(GLFWwindow *window, int width, int height) {
    printf("resize: %dx%d\n", width, height);
    glViewport(0, 0, width, height);
    if (height != 0) {
        float aspect = float(width) / height;
        projection = perspective(31.f, aspect, 1000.f, 10000.f);
        updateMatrices();
    }
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_W) {
        static bool wireframe = false;
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    } else if (key == GLFW_KEY_S) {
        part++;
        if (part >= mesh.meshParts.size()) {
            part = -1;
        }
    }
}

vec2 lastMouse = vec2(-1,-1);

static void glfw_click_callback(GLFWwindow *window, int button, int action, int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) lastMouse = vec2(x, y);
        else if (action == GLFW_RELEASE) lastMouse = vec2(-1, -1);
    }
}

static void glfw_mouse_callback(GLFWwindow *window, double xPos, double yPos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) return;
    if (lastMouse == vec2(-1,-1)) {
        lastMouse = vec2(xPos, yPos);
        return; // can't update this frame, no previous data.
    } else {
        vec2 current = vec2(xPos, yPos);
        vec2 delta = current - lastMouse;
        if (delta == vec2(0,0)) return;

        vec3 rotationVector = vec3(delta.y, delta.x, 0);
        float angle = length(delta);
        rotation = rotate(angle, rotationVector) * rotation;
        updateMatrices();

        lastMouse = current;
    }
}

void glfw_error_callback(int error, const char* description) {
    cerr << "GLFW Error: " << description << " (error " << error << ")" << endl;
}

void checkShaderError(GLuint shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) return;

    cout << "Shader Compile Failed." << endl;

    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize == 0) {
        cout << "No log found." << endl;
        return;
    }

    GLchar *log = new GLchar[logSize];

    glGetShaderInfoLog(shader, logSize, &logSize, log);

    cout << log << endl;

    delete[] log;
}

void checkLinkError(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success) return;

    cout << "Shader link failed." << endl;

    GLint logSize = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize == 0) {
        cout << "No log found." << endl;
        return;
    }

    GLchar *log = new GLchar[logSize];

    glGetProgramInfoLog(program, logSize, &logSize, log);
    cout << log << endl;

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

/**
 * Loads a texture from the filesystem into OpenGL. Returns the number of channels loaded, or 0 on failure.
 * @param texname
 * @param filename
 * @param format
 * @return
 */
int loadTexture(GLuint texname, const char *filename, int format /* = STBI_default */) {
    glBindTexture(GL_TEXTURE_2D, texname);

    int width, height, bpp;
    unsigned char *pixels = stbi_load(filename, &width, &height, &bpp, format);
    if (pixels == nullptr) {
        cout << "Failed to load image " << filename << " (" << stbi_failure_reason() << ")" << endl;
        return 0;
    }
    cout << "Loaded " << filename << ", " << height << 'x' << width << ", comp = " << bpp << endl;

    if (format != STBI_default && format != bpp) {
        cout << "Changing num channels from " << bpp << " to " << format << endl;
        bpp = format;
    }

    GLenum glFormat;
    switch(bpp) {
        case STBI_grey:
            glFormat = GL_RED;
            break;
        case STBI_grey_alpha:
            glFormat = GL_RG;
            break;
        case STBI_rgb:
            glFormat = GL_RGB;
            break;
        case STBI_rgb_alpha:
            glFormat = GL_RGBA;
            break;
        default:
            cout << "Unsupported format: " << bpp << endl;
            stbi_image_free(pixels);
            return 0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, glFormat, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(pixels);

    return bpp;
}

int main() {
    if (!glfwInit()) {
        cout << "Failed to init GLFW" << endl;
        exit(-1);
    }
    cout << "GLFW Successfully Started" << endl;

    glfwSetErrorCallback(glfw_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#ifdef APPLE
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    window = glfwCreateWindow(640, 480, "Sponza Playground", NULL, NULL);
    if (!window) {
        cout << "Failed to create window" << endl;
        exit(-1);
    }

    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_click_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_callback);

    glfwMakeContextCurrent(window);

    // If the program is crashing at glGenVertexArrays, try uncommenting this line.
    //glewExperimental = GL_TRUE;
    glewInit();

    printf("OpenGL version recieved: %s\n", glGetString(GL_VERSION));

    glfwSwapInterval(1);

    initPerformanceData();

    setup();
    checkError();

    glfwSetFramebufferSizeCallback(window, glfw_resize_callback); // do this after setup
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glfw_resize_callback(window, width, height); // call resize once with the initial size

    // make sure performance data is clean going into main loop
    markPerformanceFrame();
    printPerformanceData();
    double lastPerfPrintTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {

        {
            Perf stat("Poll events");
            glfwPollEvents();
            checkError();
        }
        {
            Perf stat("Draw");
            draw();
            checkError();
        }
        {
            Perf stat("Swap buffers");
            glfwSwapBuffers(window);
            checkError();
        }

        markPerformanceFrame();

        double now = glfwGetTime();
        if (now - lastPerfPrintTime > 10.0) {
            printPerformanceData();
            lastPerfPrintTime = now;
        }
    }

    return 0;
}
