#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum ShapeType { SPHERE_SHAPE, CYLINDER_SHAPE, BOX_SHAPE, CONE_SHAPE };
enum Mode { MODELLING, INSPECTION };
enum TransformMode { NONE, ROTATE, TRANSLATE, SCALE };

// Global shader program
GLuint shaderProgram = 0;

// Abstract base shape class
class shape_t {
public:
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> colors;
    ShapeType shapetype;
    unsigned int level;
    GLuint VAO, VBO_vertices, VBO_colors, EBO;
    std::vector<unsigned int> indices;
    
    shape_t(unsigned int tesselation_level) : level(tesselation_level), VAO(0), VBO_vertices(0), VBO_colors(0), EBO(0) {
        if (level > 4) level = 4;
    }
    
    virtual ~shape_t() {
        cleanup();
    }
    
    virtual void draw() = 0;
    
    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO_vertices);
        glGenBuffers(1, &VBO_colors);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        // Vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO_vertices);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec4), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Color buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO_colors);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
        glEnableVertexAttribArray(1);
        
        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        glBindVertexArray(0);
    }
    
    void cleanup() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO_vertices) glDeleteBuffers(1, &VBO_vertices);
        if (VBO_colors) glDeleteBuffers(1, &VBO_colors);
        if (EBO) glDeleteBuffers(1, &EBO);
        VAO = VBO_vertices = VBO_colors = EBO = 0;
    }
    
    void setColor(const glm::vec4& color) {
        for (auto& c : colors) {
            c = color;
        }
        if (VAO) {
            glBindBuffer(GL_ARRAY_BUFFER, VBO_colors);
            glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());
        }
    }
};

// Sphere implementation
class sphere_t : public shape_t {
public:
    sphere_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = SPHERE_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        int segments = 8 + level * 4; // 8, 12, 16, 20, 24 segments based on level
        
        // Generate vertices
        for (int i = 0; i <= segments; ++i) {
            float lat = M_PI * (-0.5f + (float)i / segments);
            for (int j = 0; j <= segments; ++j) {
                float lng = 2 * M_PI * (float)j / segments;
                
                float x = cos(lat) * cos(lng);
                float y = sin(lat);
                float z = cos(lat) * sin(lng);
                
                vertices.emplace_back(x, y, z, 1.0f);
                colors.emplace_back(0.8f, 0.2f, 0.2f, 1.0f); // Red color
            }
        }
        
        // Generate indices
        for (int i = 0; i < segments; ++i) {
            for (int j = 0; j < segments; ++j) {
                int first = i * (segments + 1) + j;
                int second = first + segments + 1;
                
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);
                
                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Cylinder implementation
class cylinder_t : public shape_t {
public:
    cylinder_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = CYLINDER_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        int segments = 6 + level * 3; // 6, 9, 12, 15, 18 segments based on level
        
        // Add center vertices for caps
        vertices.emplace_back(0, -1.0f, 0, 1.0f); // bottom center
        colors.emplace_back(0.2f, 0.8f, 0.2f, 1.0f); // Green
        vertices.emplace_back(0, 1.0f, 0, 1.0f);  // top center
        colors.emplace_back(0.2f, 0.8f, 0.2f, 1.0f);
        
        // Side vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -1.0f, z, 1.0f); // bottom
            vertices.emplace_back(x, 1.0f, z, 1.0f);  // top
            colors.emplace_back(0.2f, 0.8f, 0.2f, 1.0f);
            colors.emplace_back(0.2f, 0.8f, 0.2f, 1.0f);
        }
        
        // Side faces
        for (int i = 0; i < segments; ++i) {
            int base = 2 + i * 2; // offset by 2 for center vertices
            
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            
            indices.push_back(base + 1);
            indices.push_back(base + 3);
            indices.push_back(base + 2);
        }
        
        // Bottom cap
        for (int i = 0; i < segments; ++i) {
            int current = 2 + i * 2;
            int next = 2 + ((i + 1) % segments) * 2;
            
            indices.push_back(0); // bottom center
            indices.push_back(next);
            indices.push_back(current);
        }
        
        // Top cap
        for (int i = 0; i < segments; ++i) {
            int current = 2 + i * 2 + 1;
            int next = 2 + ((i + 1) % segments) * 2 + 1;
            
            indices.push_back(1); // top center
            indices.push_back(current);
            indices.push_back(next);
        }
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Box implementation
class box_t : public shape_t {
public:
    box_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = BOX_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        // Simple cube vertices
        std::vector<glm::vec3> cubeVertices = {
            // Front face
            {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
            // Back face
            {-1, -1, -1}, {-1,  1, -1}, { 1,  1, -1}, { 1, -1, -1},
            // Top face
            {-1,  1, -1}, {-1,  1,  1}, { 1,  1,  1}, { 1,  1, -1},
            // Bottom face
            {-1, -1, -1}, { 1, -1, -1}, { 1, -1,  1}, {-1, -1,  1},
            // Right face
            { 1, -1, -1}, { 1,  1, -1}, { 1,  1,  1}, { 1, -1,  1},
            // Left face
            {-1, -1, -1}, {-1, -1,  1}, {-1,  1,  1}, {-1,  1, -1}
        };
        
        for (const auto& v : cubeVertices) {
            vertices.emplace_back(v.x, v.y, v.z, 1.0f);
            colors.emplace_back(0.2f, 0.2f, 0.8f, 1.0f); // Blue
        }
        
        // Cube indices
        std::vector<unsigned int> cubeIndices = {
            0,  1,  2,   0,  2,  3,   // front
            4,  5,  6,   4,  6,  7,   // back
            8,  9, 10,   8, 10, 11,   // top
            12, 13, 14,  12, 14, 15,  // bottom
            16, 17, 18,  16, 18, 19,  // right
            20, 21, 22,  20, 22, 23   // left
        };
        
        indices = cubeIndices;
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Cone implementation
class cone_t : public shape_t {
public:
    cone_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = CONE_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        int segments = 6 + level * 3; // 6, 9, 12, 15, 18 segments based on level
        
        // Apex vertex
        vertices.emplace_back(0, 1.0f, 0, 1.0f);
        colors.emplace_back(0.8f, 0.8f, 0.2f, 1.0f); // Yellow
        
        // Base center
        vertices.emplace_back(0, -1.0f, 0, 1.0f);
        colors.emplace_back(0.8f, 0.8f, 0.2f, 1.0f);
        
        // Base vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -1.0f, z, 1.0f);
            colors.emplace_back(0.8f, 0.8f, 0.2f, 1.0f);
        }
        
        // Side triangles (apex to base edge)
        for (int i = 0; i < segments; ++i) {
            indices.push_back(0); // apex
            indices.push_back(i + 2);
            indices.push_back(i + 3);
        }
        
        // Base triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(1); // center
            indices.push_back(i + 3);
            indices.push_back(i + 2);
        }
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Hierarchical Node class
class HNode {
public:
    std::unique_ptr<shape_t> shape;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 scale;
    std::vector<std::unique_ptr<HNode>> children;
    
    HNode(std::unique_ptr<shape_t> s) : shape(std::move(s)) {
        rotation = glm::mat4(1.0f);
        translation = glm::mat4(1.0f);
        scale = glm::mat4(1.0f);
    }
    
    void render(const glm::mat4& parentTransform = glm::mat4(1.0f)) {
        glm::mat4 currentTransform = parentTransform * translation * rotation * scale;
        
        // Set model matrix uniform
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(currentTransform));
        }
        
        if (shape) {
            shape->draw();
        }
        
        // Render children
        for (auto& child : children) {
            child->render(currentTransform);
        }
    }
    
    void addChild(std::unique_ptr<HNode> child) {
        children.push_back(std::move(child));
    }
};

// Model class
class model_t {
private:
    std::vector<std::unique_ptr<HNode>> nodes;
    
public:
    void addShape(std::unique_ptr<shape_t> shape) {
        auto node = std::make_unique<HNode>(std::move(shape));
        nodes.push_back(std::move(node));
    }
    
    void removeLastShape() {
        if (!nodes.empty()) {
            nodes.pop_back();
        }
    }
    
    HNode* getLastNode() {
        return nodes.empty() ? nullptr : nodes.back().get();
    }
    
    void render() {
        for (auto& node : nodes) {
            node->render();
        }
    }
    
    size_t getShapeCount() const {
        return nodes.size();
    }
    
    void clear() {
        nodes.clear();
    }
    
    bool isEmpty() const {
        return nodes.empty();
    }
    
    // Save and load methods (simplified versions)
    void save(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to save model to " << filename << std::endl;
            return;
        }
        
        file << "MODEL_FILE_VERSION 1.0\n";
        file << "SHAPE_COUNT " << nodes.size() << "\n";
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            if (node->shape) {
                file << "SHAPE " << i << "\n";
                file << "TYPE " << static_cast<int>(node->shape->shapetype) << "\n";
                file << "LEVEL " << node->shape->level << "\n";
                
                // Save first color
                if (!node->shape->colors.empty()) {
                    const auto& color = node->shape->colors[0];
                    file << "COLOR " << color.r << " " << color.g << " " << color.b << " " << color.a << "\n";
                }
            }
        }
        
        file.close();
        std::cout << "Model saved to " << filename << std::endl;
    }
    
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to load model from " << filename << std::endl;
            return false;
        }
        
        clear();
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token == "SHAPE") {
                int shapeType = 0, level = 1;
                glm::vec4 color(0.7f, 0.7f, 0.7f, 1.0f);
                
                // Read next few lines for shape properties
                for (int i = 0; i < 3; ++i) {
                    if (std::getline(file, line)) {
                        std::istringstream propStream(line);
                        std::string prop;
                        propStream >> prop;
                        
                        if (prop == "TYPE") {
                            propStream >> shapeType;
                        } else if (prop == "LEVEL") {
                            propStream >> level;
                        } else if (prop == "COLOR") {
                            propStream >> color.r >> color.g >> color.b >> color.a;
                        }
                    }
                }
                
                // Create shape
                std::unique_ptr<shape_t> shape;
                switch (static_cast<ShapeType>(shapeType)) {
                    case SPHERE_SHAPE:
                        shape = std::make_unique<sphere_t>(level);
                        break;
                    case CYLINDER_SHAPE:
                        shape = std::make_unique<cylinder_t>(level);
                        break;
                    case BOX_SHAPE:
                        shape = std::make_unique<box_t>(level);
                        break;
                    case CONE_SHAPE:
                        shape = std::make_unique<cone_t>(level);
                        break;
                }
                
                if (shape) {
                    shape->setColor(color);
                    addShape(std::move(shape));
                }
            }
        }
        
        file.close();
        std::cout << "Model loaded from " << filename << std::endl;
        return true;
    }
};

// Global variables
Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char activeAxis = 'X';
std::unique_ptr<model_t> currentModel;
HNode* currentNode = nullptr;
glm::mat4 modelRotation = glm::mat4(1.0f);

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Shader creation functions
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Shader compilation failed: " << infoLog << std::endl;
    }
    
    return shader;
}

GLuint createShaderProgram() {
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 aPos;
    layout (location = 1) in vec4 aColor;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec4 vertexColor;
    
    void main() {
        gl_Position = projection * view * model * aPos;
        vertexColor = aColor;
    }
    )";
    
    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec4 vertexColor;
    out vec4 FragColor;
    
    void main() {
        FragColor = vertexColor;
    }
    )";
    
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Shader program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// Key handling functions
void applyTransform(int direction) {
    if (!currentNode || transformMode == NONE) return;
    
    float amount = direction * 0.1f;
    float rotAmount = direction * glm::radians(5.0f);
    float scaleAmount = direction > 0 ? 1.1f : 0.9f;
    
    glm::vec3 axis(0);
    switch (activeAxis) {
        case 'X': axis = glm::vec3(1, 0, 0); break;
        case 'Y': axis = glm::vec3(0, 1, 0); break;
        case 'Z': axis = glm::vec3(0, 0, 1); break;
    }
    
    switch (transformMode) {
        case ROTATE:
            currentNode->rotation = glm::rotate(currentNode->rotation, rotAmount, axis);
            break;
        case TRANSLATE:
            currentNode->translation = glm::translate(currentNode->translation, axis * amount);
            break;
        case SCALE:
            currentNode->scale = glm::scale(currentNode->scale, axis * (scaleAmount - 1.0f) + glm::vec3(1.0f));
            break;
    }
}

void handleModellingKeys(int key) {
    switch (key) {
        case GLFW_KEY_1:
            currentModel->addShape(std::make_unique<sphere_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Sphere added" << std::endl;
            break;
        case GLFW_KEY_2:
            currentModel->addShape(std::make_unique<cylinder_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cylinder added" << std::endl;
            break;
        case GLFW_KEY_3:
            currentModel->addShape(std::make_unique<box_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Box added" << std::endl;
            break;
        case GLFW_KEY_4:
            currentModel->addShape(std::make_unique<cone_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cone added" << std::endl;
            break;
        case GLFW_KEY_5:
            currentModel->removeLastShape();
            currentNode = currentModel->getLastNode();
            std::cout << "Last shape removed" << std::endl;
            break;
        case GLFW_KEY_R:
            transformMode = ROTATE;
            std::cout << "Transform mode: ROTATE" << std::endl;
            break;
        case GLFW_KEY_T:
            transformMode = TRANSLATE;
            std::cout << "Transform mode: TRANSLATE" << std::endl;
            break;
        case GLFW_KEY_G:
            transformMode = SCALE;
            std::cout << "Transform mode: SCALE" << std::endl;
            break;
        case GLFW_KEY_X:
            activeAxis = 'X';
            std::cout << "Active Axis: X" << std::endl;
            break;
        case GLFW_KEY_Y:
            activeAxis = 'Y';
            std::cout << "Active Axis: Y" << std::endl;
            break;
        case GLFW_KEY_Z:
            activeAxis = 'Z';
            std::cout << "Active Axis: Z" << std::endl;
            break;
        case GLFW_KEY_KP_ADD:
        case GLFW_KEY_EQUAL:
            applyTransform(+1);
            break;
        case GLFW_KEY_KP_SUBTRACT:
        case GLFW_KEY_MINUS:
            applyTransform(-1);
            break;
        case GLFW_KEY_C: {
            if (currentNode && currentNode->shape) {
                float r, g, b;
                std::cout << "Enter RGB values (0-1): ";
                std::cin >> r >> g >> b;
                currentNode->shape->setColor(glm::vec4(r, g, b, 1.0f));
                std::cout << "Color changed" << std::endl;
            }
            break;
        }
        case GLFW_KEY_S: {
            std::string filename;
            std::cout << "Enter filename (with .mod extension): ";
            std::cin >> filename;
            if (filename.find(".mod") == std::string::npos) {
                filename += ".mod";
            }
            currentModel->save(filename);
            break;
        }
    }
}

void handleInspectionKeys(int key) {
    switch (key) {
        case GLFW_KEY_L: {
            std::string filename;
            std::cout << "Enter filename to load: ";
            std::cin >> filename;
            if (currentModel->load(filename)) {
                currentNode = currentModel->getLastNode();
                modelRotation = glm::mat4(1.0f);
            }
            break;
        }
        case GLFW_KEY_R:
            transformMode = ROTATE;
            std::cout << "Model rotation mode activated" << std::endl;
            break;
        case GLFW_KEY_X:
            activeAxis = 'X';
            std::cout << "Model rotation axis: X" << std::endl;
            break;
        case GLFW_KEY_Y:
            activeAxis = 'Y';
            std::cout << "Model rotation axis: Y" << std::endl;
            break;
        case GLFW_KEY_Z:
            activeAxis = 'Z';
            std::cout << "Model rotation axis: Z" << std::endl;
            break;
        case GLFW_KEY_KP_ADD:
        case GLFW_KEY_EQUAL:
            if (transformMode == ROTATE) {
                float angle = glm::radians(5.0f);
                switch (activeAxis) {
                    case 'X': modelRotation = glm::rotate(modelRotation, angle, glm::vec3(1, 0, 0)); break;
                    case 'Y': modelRotation = glm::rotate(modelRotation, angle, glm::vec3(0, 1, 0)); break;
                    case 'Z': modelRotation = glm::rotate(modelRotation, angle, glm::vec3(0, 0, 1)); break;
                }
            }
            break;
        case GLFW_KEY_KP_SUBTRACT:
        case GLFW_KEY_MINUS:
            if (transformMode == ROTATE) {
                float angle = glm::radians(-5.0f);
                switch (activeAxis) {
                    case 'X': modelRotation = glm::rotate(modelRotation, angle, glm::vec3(1, 0, 0)); break;
                    case 'Y': modelRotation = glm::rotate(modelRotation, angle, glm::vec3(0, 1, 0)); break;
                    case 'Z': modelRotation = glm::rotate(modelRotation, angle, glm::vec3(0, 0, 1)); break;
                }
            }
            break;
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_M) {
        currentMode = MODELLING;
        transformMode = NONE;
        std::cout << "Mode: MODELLING" << std::endl;
    }
    else if (key == GLFW_KEY_I) {
        currentMode = INSPECTION;
        transformMode = NONE;
        std::cout << "Mode: INSPECTION" << std::endl;
    }
    else if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (currentMode == MODELLING) {
        handleModellingKeys(key);
    } else if (currentMode == INSPECTION) {
        handleInspectionKeys(key);
    }
}

void printInstructions() {
    std::cout << "\n=== 3D MODELLER CONTROLS ===" << std::endl;
    std::cout << "Mode switching:" << std::endl;
    std::cout << "  M - Modeling Mode" << std::endl;
    std::cout << "  I - Inspection Mode" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    
    std::cout << "\nModeling Mode:" << std::endl;
    std::cout << "  1-4 - Add sphere/cylinder/box/cone" << std::endl;
    std::cout << "  5 - Remove last shape" << std::endl;
    std::cout << "  R - Rotation mode" << std::endl;
    std::cout << "  T - Translation mode" << std::endl;
    std::cout << "  G - Scaling mode" << std::endl;
    std::cout << "  C - Change color" << std::endl;
    std::cout << "  S - Save model" << std::endl;
    
    std::cout << "\nInspection Mode:" << std::endl;
    std::cout << "  L - Load model" << std::endl;
    std::cout << "  R - Rotate model" << std::endl;
    
    std::cout << "\nTransform Controls:" << std::endl;
    std::cout << "  X/Y/Z - Select axis" << std::endl;
    std::cout << "  +/- - Apply transformation" << std::endl;
    std::cout << "==============================\n" << std::endl;
}

void setupOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void updateCamera() {
    // For inspection mode, apply model rotation
    glm::vec3 finalCameraPos = cameraPos;
    glm::vec3 finalCameraTarget = cameraTarget;
    
    if (currentMode == INSPECTION) {
        // Apply model rotation to camera position
        glm::vec4 rotatedPos = modelRotation * glm::vec4(cameraPos, 1.0f);
        finalCameraPos = glm::vec3(rotatedPos);
    }
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(shaderProgram);
    
    // Set up view and projection matrices
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1024.0f/768.0f, 0.1f, 100.0f);
    
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (projLoc != -1) {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    
    // Apply global model rotation for inspection mode
    if (currentMode == INSPECTION) {
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelRotation));
        }
    }
    
    // Render the model
    if (currentModel && !currentModel->isEmpty()) {
        currentModel->render();
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1024, 768, "3D Modeller", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    // Setup OpenGL
    setupOpenGL();
    
    // Create shader program
    shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        std::cout << "Failed to create shader program" << std::endl;
        return -1;
    }
    
    // Initialize model
    currentModel = std::make_unique<model_t>();
    
    std::cout << "3D Modeller initialized successfully!" << std::endl;
    printInstructions();
    std::cout << "Mode: MODELLING" << std::endl;
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderScene();
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    currentModel.reset();
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    glfwTerminate();
    
    std::cout << "Application terminated successfully." << std::endl;
    return 0;
}
