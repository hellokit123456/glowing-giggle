#include <GL/glew.h>     // MUST come first
#include <GLFW/glfw3.h>  // then GLFW
#include <glm/glm.hpp>   // then 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <string>
#include <sstream>
#include "shape.h"
#include "input.h"
#include "globals.h"
#include "HIERARCHIAL.h"
glm::mat4 projection;
glm::mat4 view;
Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char activeAxis = 'X';
std::shared_ptr<model_t> currentModel=nullptr;
std::shared_ptr<model_node_t> currentNode ;
GLuint shaderProgram = 0;
// Camera variables
float cameraDistance = 5.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
glm::mat4 modelRotation = glm::mat4(1.0f);

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



void renderNode(model_node_t* node, const glm::mat4& parentTransform) {
    glm::mat4 modelMatrix = parentTransform * node->getTransform();

    if (node->shape) {
        glm::mat4 MVP = projection * view * modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"),
                           1, GL_FALSE, glm::value_ptr(MVP));
        node->shape->draw();
    }

    for (auto& child : node->children) {
        renderNode(child.get(), modelMatrix);
    }
}
void checkOpenGLError(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL Error after " << operation << ": " << error << std::endl;
    }
}
void renderScene(){
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    
    GLuint mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
    
    // Just render one big cylinder at the origin
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix - no transform
    glm::mat4 mvp = projection * view * model;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    cylinder_t cylinder(2);  // tesselation level 2
    std::cout << "Cylinder vertices: " << cylinder.vertices.size() << std::endl;
    std::cout << "Cylinder indices: " << cylinder.indices.size() << std::endl;
    std::cout << "Expected max index: " << cylinder.vertices.size() - 1 << std::endl;
    std::cout << "MVP location: " << mvpLoc << std::endl; // Check if uniform is found
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // Wireframe mode
cylinder.draw();
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // Back to solid
    cylinder.draw();
    // After cylinder creation in main():

}
int main() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
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

 


// Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Dark green background
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Shaders
    shaderProgram = createShaderProgram();
    GLint success;
glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "Shader program linking failed: " << infoLog << std::endl;
} else {
    std::cout << "Shaders compiled and linked successfully!" << std::endl;
}
    // Camera
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f),
                       glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));

    // Scene + keyboard
    currentModel = std::make_shared<model_t>();
    glfwSetKeyCallback(window, keyCallback);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Dark green background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkOpenGLError("clear");
        glUseProgram(shaderProgram);
        renderScene();
        
          
    glfwSwapBuffers(window);
    glfwPollEvents();
}
        // Simple triangle vertices
float triangleVertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};
// Add this in your main() after creating the cylinder:
float quadVertices[] = {
    -0.5f, -0.5f, 0.0f, 1.0f,  // position
     1.0f,  0.0f, 0.0f, 1.0f,  // red color
     
     0.5f, -0.5f, 0.0f, 1.0f,  // position  
     0.0f,  1.0f, 0.0f, 1.0f,  // green color
     
     0.0f,  0.5f, 0.0f, 1.0f,  // position
     0.0f,  0.0f, 1.0f, 1.0f   // blue color
};

GLuint quadVAO, quadVBO;
glGenVertexArrays(1, &quadVAO);
glGenBuffers(1, &quadVBO);
glBindVertexArray(quadVAO);
glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

// Position attribute
glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// Color attribute  
glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
glEnableVertexAttribArray(1);

// In renderScene(), try this instead:
glBindVertexArray(quadVAO);
glDrawArrays(GL_TRIANGLES, 0, 3);
GLuint testVAO, testVBO;
glGenVertexArrays(1, &testVAO);
glGenBuffers(1, &testVBO);
glBindVertexArray(testVAO);
glBindBuffer(GL_ARRAY_BUFFER, testVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);


    

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}





