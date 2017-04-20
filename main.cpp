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

using namespace std;
using namespace glm;

int loadTexture(GLuint texname, const char *filename, int format = STBI_default);

GLFWwindow *window;

mat4 rotation; // identity
mat4 view;
mat4 projection;

OBJMesh mesh;

GLuint meshVao, testVao;

bool materialPreview = false;

int part = -1;
int renderMode = 0;

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

    meshVao = createVao();
    glBufferData(GL_ARRAY_BUFFER, mesh.verts.size() * sizeof(mesh.verts[0]), mesh.verts.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(mesh.indices[0]), mesh.indices.data(), GL_STATIC_DRAW);
    checkError();

    float testVerts[] = {
        0, 0, 0,
        0, 0, 1,
        1, 0,

       -1, 0, 0,
        0, 0, 1,
        0, 0,

       -1,-1, 0,
        0, 0, 1,
        0, 1,

        0,-1, 0,
        0, 0, 1,
        1, 1,
    };

    u32 testIndices[] = {0, 1, 2, 2, 3, 0};

    testVao = createVao();
    glBufferData(GL_ARRAY_BUFFER, sizeof(testVerts), testVerts, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(testIndices), testIndices, GL_STATIC_DRAW);
    checkError();

    initShaders();

    view = lookAt(vec3(0, 0, 7000), vec3(0), vec3(0, 1, 0));
    rotation = lookAt(vec3(0), vec3(-1), vec3(0, 1, 0));
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 mvp = projection * view * rotation;
    mat3 normal = mat3(view * rotation);

    glBindVertexArray(meshVao);
    if (part == -1) {
        if (renderMode == 2) {
            for (int c = 0, n = mesh.meshParts.size(); c < n; c++) {
                OBJMeshPart &mp = mesh.meshParts[c];
                OBJMaterial &mat = mesh.materials[mp.materialIndex];
                Shader &shader = findShader(mesh, mat); // TODO: Cache this
                bindShader(shader);
                bindMaterial(mvp, normal, mesh, mat);
                glDrawElements(GL_TRIANGLES, mp.indexSize, GL_UNSIGNED_INT, (void *)(mp.indexOffset * sizeof(u32)));
            }
        } else {
            // screw mesh parts, just draw everything.
            bindShader(getShader(renderMode));
            bindMaterial(mvp, normal, mesh, mesh.materials[0]);
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        }
    } else {
        OBJMeshPart &mp = mesh.meshParts[part];
        OBJMaterial &mat = mesh.materials[mp.materialIndex];
        Shader &shader = renderMode == 2 ? findShader(mesh, mat) : getShader(renderMode);
        bindShader(shader);
        bindMaterial(mvp, normal, mesh, mat);
        glDrawElements(GL_TRIANGLES, mp.indexSize, GL_UNSIGNED_INT, (void *)(mp.indexOffset * sizeof(u32)));

        if (materialPreview) {
            glBindVertexArray(testVao);
            mat4 idt4(1);
            mat3 idt3(1);
            bindMaterial(idt4, idt3, mesh, mat);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
    }
}

static void glfw_resize_callback(GLFWwindow *window, int width, int height) {
    printf("resize: %dx%d\n", width, height);
    glViewport(0, 0, width, height);
    if (height != 0) {
        float aspect = float(width) / height;
        projection = perspective(31.f, aspect, 1000.f, 10000.f);
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
    } else if (key == GLFW_KEY_M) {
        renderMode++;
        if (renderMode >= 3) {
            renderMode = 0;
        }
    } else if (key == GLFW_KEY_P) {
        materialPreview = !materialPreview;
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

        lastMouse = current;
    }
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
