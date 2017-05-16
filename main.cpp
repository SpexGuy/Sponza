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
#include "material.h"
#include "camera.h"

using namespace std;
using namespace glm;

int loadTexture(GLuint texname, const char *filename, int format = STBI_default);

GLFWwindow *window;

OrbitNoGimbleCamera orbitCam;
FlyAroundCamera flyCam;

Camera *cameras[] = {
    &flyCam,
    &orbitCam
};
int currentCamera = 0;
const int nCameras = sizeof(cameras) / sizeof(cameras[0]);

bool cursorCaught;

mat4 projection;

Mesh mesh;

GLuint testVao;

bool materialPreview = false;

int part = -1;
int renderMode = 0;

void loadTextures(const string &dir, OBJMesh &obj) {
    for (OBJTexture &tex : obj.textures) {
        if (tex.texName == UNLOADED) {
            string file = dir + '/' + tex.name;
            glGenTextures(1, &tex.texName);
            if (!loadTexture(tex.texName, file.c_str(), STBI_rgb)) {
                glDeleteTextures(1, &tex.texName);
                tex.texName = UNLOADED;
            }
        }
    }
}

void setup() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorCaught = true;

    initShaders();

    OBJMesh obj;
    bool success = loadObjFile("assets/sponza", "sponza.obj", obj);
    if (!success) {
        printf("Failed to load materials.\n");
        exit(2);
    }
    printf("Loaded %lu materials.\n", obj.materials.size());
    printf("Loaded %lu mesh parts.\n", obj.meshParts.size());
    printf("Loaded %lu vertices and %lu indices.\n", obj.verts.size(), obj.indices.size());

    loadTextures("assets/sponza", obj);

    obj2mesh(obj, mesh);

    float testVerts[] = {
        0, 0, 0,    // position
        0, 0, 1,    // normal
        1, 0, 0,    // tangent
        0, 1, 0,    // bitangent
        1, 0,       // tex

       -1, 0, 0,
        0, 0, 1,
        1, 0, 0,
        0, 1, 0,
        0, 0,

       -1,-1, 0,
        0, 0, 1,
        1, 0, 0,
        0, 1, 0,
        0, 1,

        0,-1, 0,
        0, 0, 1,
        1, 0, 0,
        0, 1, 0,
        1, 1,
    };

    u32 testIndices[] = {0, 1, 2, 2, 3, 0};

    testVao = createVao();
    glBufferData(GL_ARRAY_BUFFER, sizeof(testVerts), testVerts, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(testIndices), testIndices, GL_STATIC_DRAW);
    checkError();

    orbitCam.m_offset = lookAt(vec3(0, 0, 7000), vec3(0), vec3(0, 1, 0));
    orbitCam.m_rotation = lookAt(vec3(0), vec3(-1), vec3(0, 1, 0));
    orbitCam.m_dirty = true;
    orbitCam.update(0); // setup orbitCam.m_pos for lighting
    flyCam.m_pos = vec3(0, 200, 0);
}

void draw(s32 dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Camera *cam = cameras[currentCamera];
    cam->update(dt);
    mat4 mv = cam->m_view;
    mat4 mvp = projection * mv;
    vec3 camPos = cam->m_pos;
    vec3 lightPos = orbitCam.m_pos;

    glBindVertexArray(mesh.vao);
    if (part == -1) {
        if (renderMode == kDiffuseTex) {
            for (int c = 0, n = mesh.parts.size(); c < n; c++) {
                MeshPart &mp = mesh.parts[c];
                Material &mat = mesh.materials[mp.material];
                bindShader(mp.shader);
                bindMaterial(mvp, camPos, lightPos, mesh, mat);
                glDrawElements(GL_TRIANGLES, mp.size, GL_UNSIGNED_INT, (void *)(mp.offset * sizeof(u32)));
            }
        } else {
            // screw mesh parts, just draw everything.
            bindShader(renderMode);
            bindMaterial(mvp, camPos, lightPos, mesh, mesh.materials[0]);
            glDrawElements(GL_TRIANGLES, mesh.size, GL_UNSIGNED_INT, 0);
        }
    } else {
        MeshPart &mp = mesh.parts[part];
        Material &mat = mesh.materials[mp.material];
        u16 shader = renderMode == 2 ? mp.shader : u16(renderMode);
        bindShader(shader);
        bindMaterial(mvp, camPos, lightPos, mesh, mat);
        glDrawElements(GL_TRIANGLES, mp.size, GL_UNSIGNED_INT, (void *)(mp.offset * sizeof(u32)));

        if (materialPreview) {
            glBindVertexArray(testVao);
            mat4 idt4(1);
            bindMaterial(idt4, vec3(0,0,1), lightPos, mesh, mat);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
    }
}

static void glfw_resize_callback(GLFWwindow *window, int width, int height) {
    printf("resize: %dx%d\n", width, height);
    glViewport(0, 0, width, height);
    if (height != 0) {
        float aspect = float(width) / height;
        projection = perspective(31.f, aspect, 10.f, 10000.f);
    }
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_R) {
        static bool wireframe = false;
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    } else if (key == GLFW_KEY_F) {
        part++;
        if (part >= mesh.parts.size()) {
            part = -1;
        }
    } else if (key == GLFW_KEY_M) {
        renderMode++;
        if (renderMode > kDiffuseTex) {
            renderMode = 0;
        }
    } else if (key == GLFW_KEY_P) {
        materialPreview = !materialPreview;
    } else if (key == GLFW_KEY_C) {
        currentCamera++;
        if (currentCamera >= nCameras) {
            currentCamera = 0;
        }
    }
}

vec2 lastMouse = vec2(-1,-1);

static void glfw_click_callback(GLFWwindow *window, int button, int action, int mods) {
//    double x, y;
//    glfwGetCursorPos(window, &x, &y);

    if (action != GLFW_PRESS) return;

    if (cursorCaught) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        cursorCaught = false;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cursorCaught = true;
    }
}

static void glfw_mouse_callback(GLFWwindow *window, double xPos, double yPos) {
    if (lastMouse == vec2(-1,-1)) {
        lastMouse = vec2(xPos, yPos);
        return; // can't update this frame, no previous data.
    }

    vec2 current = vec2(xPos, yPos);
    vec2 delta = current - lastMouse;
    if (delta == vec2(0,0)) return;

    if (cursorCaught) {
        cameras[currentCamera]->mouseMoved(delta);
    }

    lastMouse = current;
}

void glfw_error_callback(int error, const char* description) {
    cerr << "GLFW Error: " << description << " (error " << error << ")" << endl;
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

    stbi_image_free(pixels);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    clock_t lastTime = clock() * 1000 / CLOCKS_PER_SEC;
    while (!glfwWindowShouldClose(window)) {

        {
            Perf stat("Poll events");
            glfwPollEvents();
            checkError();
        }
        {
            Perf stat("Draw");
            clock_t now = clock() * 1000 / CLOCKS_PER_SEC;
            s32 dt = s32(now - lastTime);
            draw(dt);
            lastTime = now;
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
