#include <GL/glew.h>     // MUST come first
#include <GLFW/glfw3.h>  // then GLFW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "shape.h"
#include "input.h"
#include "globals.h"
#include "HIERARCHIAL.h"

// --- Global Variable DEFINITIONS ---
// These were previously missing
glm::mat4 projection;
glm::mat4 view;
GLuint shaderProgram = 0;
Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char activeAxis = 'X';
std::shared_ptr<model_t> currentModel;
std::shared_ptr<model_node_t> currentNode;
float cameraDistance = 5.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
glm::mat4 modelRotation = glm::mat4(1.0f);


// --- Shader Creation ---
GLuint createShaderProgram() {
    const char* vertexShaderSrc = R"(
    #version 330 core
    layout(location = 0) in vec4 aPos;
    layout(location = 1) in vec4 aColor;
    uniform mat4 MVP;
    out vec4 fragColor;
    void main() {
        gl_Position = MVP * aPos;
        fragColor = aColor;
    })";

    const char* fragmentShaderSrc = R"(
    #version 330 core
    in vec4 fragColor;
    out vec4 color;
    void main() {
        color = fragColor;
    })";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


// --- Rendering Logic ---
void renderNode(std::shared_ptr<model_node_t> node, const glm::mat4& parentTransform) {
    if (!node) return;
    glm::mat4 modelMatrix = parentTransform * node->getTransform();

    if (node->shape) {
        glm::mat4 MVP = projection * view * modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"),
                           1, GL_FALSE, glm::value_ptr(MVP));
        node->shape->draw(MVP, shaderProgram);
    }

    for (auto& child : node->children) {
        renderNode(child, modelMatrix);
    }
}

void renderScene() {
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
    if (currentMode == INSPECTION) {
        view = glm::lookAt(
            glm::vec3(cameraDistance * sin(glm::radians(cameraAngleY)) * cos(glm::radians(cameraAngleX)),
                      cameraDistance * sin(glm::radians(cameraAngleX)),
                      cameraDistance * cos(glm::radians(cameraAngleY)) * cos(glm::radians(cameraAngleX))),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        if (currentModel && currentModel->getRoot()) {
            renderNode(currentModel->getRoot(), modelRotation);
        }
    } else {
        view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f),
                          glm::vec3(0.0f, 0.0f, 0.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
        if (currentModel && currentModel->getRoot()) {
            renderNode(currentModel->getRoot(), glm::mat4(1.0f));
        }
    }
}


// --- Main Application ---
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Modeller", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        std::cerr << "Failed to create shader program\n";
        return -1;
    }
    std::cout << "Shaders compiled and linked successfully!" << std::endl;
    
    currentModel = std::make_shared<model_t>();
    currentNode = currentModel->getRoot();
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        renderScene();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}