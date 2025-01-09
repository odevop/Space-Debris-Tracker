/****************************************************************/
/*                   OpenGL Implementation(Header)              */
/*                           Blake Owen                         */
/*        This implementation was refactored and modified from  */
/*        the OpenGL sphere example provided by songho:         */
/*        http://www.songho.ca/opengl/gl_sphere.html            */
/*                                                              */
/****************************************************************/

// https://github.com/ocornut/imgui
#include "imgui.h"

// http://www.songho.ca/opengl/gl_sphere.html
#include "Sphere.h"
#include "Bmp.h"
#include "BitmapFontData.h"
#include "Matrices.h"

// https://www.glfw.org/
#include <gl.h>
#define GLFW_INCLUDE_NONE
#include <glfw3.h>

#include <iostream>
#include <vector>
#include <unordered_set>

#include "TLEReader.h"
#include "SpaceDebris.h"

#pragma once

struct xyz {
    float x;
    float y;
    float z;
};

class OpenGLEngine {
    public:
    // Constructor
    OpenGLEngine();

    // Window
    GLFWwindow* window;

    void init();

    void preFrame(double frameTime);
    void frame(double frameTime);
    void postFrame(double frameTime);
    void toPerspective();
    void toOrtho();
    void initGL();
    bool initGLSL();
    void initVBO();
    bool initSharedMem();
    void clearSharedMem();
    GLuint loadTexture(const char* fileName, bool wrap=true);
    void showInfo();
    void showFPS();

    void mainEventLoop();
    void shutdown();

    private:
    // Constants
    const int   WINDOW_WIDTH    = 1280;
    const int   WINDOW_HEIGHT   = 720;
    const float CAMERA_DISTANCE = 4.0f;

    // Global Variables
    int windowWidth;
    int windowHeight;
    int fbWidth;
    int fbHeight;
    int frameCounter;
    double runTime;
    double totalTime;
    bool mouseLeftDown;
    bool mouseRightDown;
    bool mouseMiddleDown;
    double mouseX, mouseY;
    float cameraAngleX;
    float cameraAngleY;
    float cameraDistance;
    int drawMode;
    bool isValid;
    
    Datetime newTime;
    float* sunAngle;
    xyz cameraCenter;

    // ImGUI
    char day[3] = {'\0'};
    char month[3] = {'\0'};
    char year[5] = {'\0'};

    char hours[3] = {'\0'};
    char minutes[3] = {'\0'};
    char seconds[3] = {'\0'};

    // VAO / VBO
    GLuint vao;
    GLuint pointsVao;
    GLuint riskyVao;
    GLuint vbo;
    GLuint pointsVbo;
    GLuint riskyVbo;
    GLuint ibo;
    GLuint texId;
    BitmapFontData bmFont;
    Matrix4 matrixModelView;
    Matrix4 matrixProjection;

    // GLSL
    GLuint progId = 0;
    GLint viewLoc;
    GLint uniformMatrixModelView;
    GLint uniformMatrixModelViewProjection;
    GLint uniformMatrixNormal;
    GLint uniformLightDirection;
    GLint uniformLightAmbient;
    GLint uniformLightDiffuse;
    GLint uniformLightSpecular;
    GLint uniformMaterialAmbient;
    GLint uniformMaterialDiffuse;
    GLint uniformMaterialSpecular;
    GLint uniformMaterialShininess;
    GLint uniformMap0;
    GLint uniformTextureUsed;
    GLint attribVertexPosition;
    GLint attribVertexNormal;
    GLint attribVertexTexCoord;

    // Objects
    Sphere earth;

    TLEReader tle;

    // Point arrays
    GLfloat* points;
    GLfloat* riskyPoints;
    GLfloat* selectedPoint;
    int numRisky;

    // Debris objects for risk calculations
    vector<SpaceDebris> debris;
    unordered_set<int> uniqueSatIds;
    vector<SpaceDebris> riskList;

    float* tolerance;
    int* iterations;

    int numSats;
    double epoch;
    int simSpeed;
    bool isPaused;

    // Current selected algorithm for risk assessment
    int algorithmSelection;
    
    // Bui-Tuong Phong shading model with texture (modified for points)
    const char* vsSource = R"(
    // GLSL version (OpenGL 3.3)
    #version 330
    // uniforms
    uniform mat4 matrixModelView;
    uniform mat4 matrixNormal;
    uniform mat4 matrixModelViewProjection;

    // vertex attribs (input)
    layout(location=0) in vec3 vertexPosition;
    layout(location=1) in vec3 vertexNormal;
    layout(location=2) in vec2 vertexTexCoord;

    // varyings (output)
    // out vec3 lightDirection;
    out vec3 esVertex;
    out vec3 esNormal;
    out vec2 texCoord0;
    void main()
    {
        // lightDirection = (mat3(view) * lightDir);
        esVertex = vec3(matrixModelView * vec4(vertexPosition, 1.0));
        esNormal = vec3(matrixNormal * vec4(vertexNormal, 1.0));
        texCoord0 = vertexTexCoord;
        gl_Position = matrixModelViewProjection * vec4(vertexPosition, 1.0);
    }
    )";

    // Fragment shader
    const char* fsSource = R"(
    // GLSL version (OpenGL 3.3)
    #version 330
    // uniforms           // should be in the eye space
    uniform vec4 lightAmbient;              // light ambient color
    uniform vec4 lightDiffuse;              // light diffuse color
    uniform vec4 lightSpecular;             // light specular color
    uniform vec4 materialAmbient;           // material ambient color
    uniform vec4 materialDiffuse;           // material diffuse color
    uniform vec4 materialSpecular;          // material specular color
    uniform float materialShininess;        // material specular shininess
    uniform sampler2D map0;                 // texture map #1
    uniform bool textureUsed;               // flag for texture
    
    // varyings (input)
    in vec3 esVertex;
    in vec3 esNormal;
    in vec2 texCoord0;

    uniform vec4 lightDirection;

    // used if object is point
    uniform bool isPoint;
    uniform bool isRisky;
    uniform bool isSelected;

    // output
    out vec4 fragColor;
    void main()
    {
        vec3 normal = normalize(esNormal);
        vec3 light;

        vec4 lightDir = normalize(-lightDirection);

        if(lightDirection.w == 0.0)
        {
            light = normalize(lightDirection.xyz);
        }
        else
        {
            light = normalize(lightDirection.xyz - esVertex);
        }

        vec3 view = normalize(-esVertex);
        vec3 reflectVec = reflect(-light, normal);  // 2 * N * (N dot L) - L

        vec3 color = lightAmbient.rgb * materialAmbient.rgb;        // begin with ambient
        float dotNL = max(dot(normal, light), 0.0);
        color += lightDiffuse.rgb * materialDiffuse.rgb * dotNL;    // add diffuse
        if(textureUsed)
            color *= texture(map0, texCoord0).rgb;                  // modulate texture map
        float dotVR = max(dot(view, reflectVec), 0.0);
        color += pow(dotVR, materialShininess) * lightSpecular.rgb * materialSpecular.rgb; // add specular
        if (isPoint) {
            fragColor = vec4(1.0, 1.0, 1.0, 1.0);
        } else if (isSelected) {
            fragColor = vec4(0.0, 1.0, 0.0, 1.0);
        } else if (isRisky) {
            fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        } else {
            fragColor = vec4(color, materialDiffuse.a);                 // set frag color
        }
        
    }
    )";

    // GLFW Callbacks
    void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void cursorPosCallback(GLFWwindow* window, double x, double y);
    void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    // GLFW Callback Wrappers
    static void framebufferSizeCallbackWrapper(GLFWwindow* window, int width, int height) {
        OpenGLEngine* engine = static_cast<OpenGLEngine*>(glfwGetWindowUserPointer(window));
        if (engine) {
            engine->framebufferSizeCallback(window, width, height);
        }
    }

    static void keyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
        OpenGLEngine* engine = static_cast<OpenGLEngine*>(glfwGetWindowUserPointer(window));
        if (engine) {
            engine->keyCallback(window, key, scancode, action, mods);
        }
    }

    static void mouseButtonCallbackWrapper(GLFWwindow* window, int button, int action, int mods) {
        OpenGLEngine* engine = static_cast<OpenGLEngine*>(glfwGetWindowUserPointer(window));
        if (engine) {
            engine->mouseButtonCallback(window, button, action, mods);
        }
    }

    static void cursorPosCallbackWrapper(GLFWwindow* window, double x, double y) {
        OpenGLEngine* engine = static_cast<OpenGLEngine*>(glfwGetWindowUserPointer(window));
        if (engine) {
            engine->cursorPosCallback(window, x, y);
        }
    }

    static void scrollCallbackWrapper(GLFWwindow* window, double xoffset, double yoffset) {
        OpenGLEngine* engine = static_cast<OpenGLEngine*>(glfwGetWindowUserPointer(window));
        if (engine) {
            engine->scrollCallback(window, xoffset, yoffset);
        }
    }

    static void errorCallback(int error, const char* description) {
        std::cout << "[ERROR]: " << description << std::endl;
    }
};
