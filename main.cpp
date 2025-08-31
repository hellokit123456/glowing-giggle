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

enum ShapeType { SPHERE_SHAPE, CYLINDER_SHAPE, BOX_SHAPE, CONE_SHAPE };
enum Mode { MODELLING, INSPECTION };
enum TransformMode { NONE, ROTATE, TRANSLATE, SCALE };

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

// Hierarchical Node class (based on Tutorial 7)
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
        
        glPushMatrix();
        glMultMatrixf(glm::value_ptr(currentTransform));
        
        if (shape) {
            shape->draw();
        }
        
        glPopMatrix();
        
        // Render children
        for (auto& child : children) {
            child->render(currentTransform);
        }
    }
    
    void addChild(std::unique_ptr<HNode> child) {
        children.push_back(std::move(child));
    }
};

// Model class using hierarchical nodes
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
    
    // Save model to file
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
                
                // Save transformation matrices
                file << "TRANSLATION ";
                for (int j = 0; j < 16; ++j) {
                    file << glm::value_ptr(node->translation)[j] << " ";
                }
                file << "\n";
                
                file << "ROTATION ";
                for (int j = 0; j < 16; ++j) {
                    file << glm::value_ptr(node->rotation)[j] << " ";
                }
                file << "\n";
                
                file << "SCALE ";
                for (int j = 0; j < 16; ++j) {
                    file << glm::value_ptr(node->scale)[j] << " ";
                }
                file << "\n";
                
                // Save color (first color in the colors vector)
                if (!node->shape->colors.empty()) {
                    const auto& color = node->shape->colors[0];
                    file << "COLOR " << color.r << " " << color.g << " " << color.b << " " << color.a << "\n";
                }
            }
        }
        
        file.close();
        std::cout << "Model saved to " << filename << std::endl;
    }
    
    // Load model from file
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to load model from " << filename << std::endl;
            return false;
        }
        
        clear(); // Remove existing model
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token == "SHAPE") {
                int shapeIndex;
                iss >> shapeIndex;
                
                // Read shape properties
                int shapeType, level;
                glm::mat4 translation(1.0f), rotation(1.0f), scale(1.0f);
                glm::vec4 color(0.7f, 0.7f, 0.7f, 1.0f);
                
                while (std::getline(file, line) && !line.empty()) {
                    std::istringstream propStream(line);
                    std::string prop;
                    propStream >> prop;
                    
                    if (prop == "TYPE") {
                        propStream >> shapeType;
                    } else if (prop == "LEVEL") {
                        propStream >> level;
                    } else if (prop == "TRANSLATION") {
                        float* ptr = glm::value_ptr(translation);
                        for (int i = 0; i < 16; ++i) {
                            propStream >> ptr[i];
                        }
                    } else if (prop == "ROTATION") {
                        float* ptr = glm::value_ptr(rotation);
                        for (int i = 0; i < 16; ++i) {
                            propStream >> ptr[i];
                        }
                    } else if (prop == "SCALE") {
                        float* ptr = glm::value_ptr(scale);
                        for (int i = 0; i < 16; ++i) {
                            propStream >> ptr[i];
                        }
                    } else if (prop == "COLOR") {
                        propStream >> color.r >> color.g >> color.b >> color.a;
                    }
                }
                
                // Create shape based on type
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
                    auto node = std::make_unique<HNode>(std::move(shape));
                    node->translation = translation;
                    node->rotation = rotation;
                    node->scale = scale;
                    nodes.push_back(std::move(node));
                }
            }
        }
        
        file.close();
        std::cout << "Model loaded from " << filename << std::endl;
        return true;
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
                colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
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
        
        // Side vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -0.5f, z, 1.0f); // bottom
            vertices.emplace_back(x, 0.5f, z, 1.0f);  // top
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        }
        
        // Side indices
        for (int i = 0; i < segments; ++i) {
            int base = i * 2;
            
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            
            indices.push_back(base + 1);
            indices.push_back(base + 3);
            indices.push_back(base + 2);
        }
        
        // Cap vertices and indices
        int centerBottom = vertices.size();
        vertices.emplace_back(0, -0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        int centerTop = vertices.size();
        vertices.emplace_back(0, 0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        for (int i = 0; i < segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -0.5f, z, 1.0f);
            vertices.emplace_back(x, 0.5f, z, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            
            int bottomVertex = centerBottom + 2 + i * 2;
            int topVertex = bottomVertex + 1;
            int nextBottomVertex = centerBottom + 2 + ((i + 1) % segments) * 2;
            int nextTopVertex = nextBottomVertex + 1;
            
            // Bottom cap
            indices.push_back(centerBottom);
            indices.push_back(nextBottomVertex);
            indices.push_back(bottomVertex);
            
            // Top cap
            indices.push_back(centerTop);
            indices.push_back(topVertex);
            indices.push_back(nextTopVertex);
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
        
        // For a box, tessellation affects subdivision of faces
        int subdivisions = level + 1; // 1, 2, 3, 4, 5 subdivisions based on level
        
        // Generate vertices for each face with subdivisions
        generateFace(glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), subdivisions); // Front
        generateFace(glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), subdivisions); // Back
        generateFace(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), subdivisions); // Top
        generateFace(glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), subdivisions); // Bottom
        generateFace(glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), subdivisions); // Right
        generateFace(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), subdivisions); // Left
    }
    
    void generateFace(const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, int subdivisions) {
        int startVertex = vertices.size();
        glm::vec3 center = normal * 0.5f;
        
        // Generate grid of vertices for this face
        for (int i = 0; i <= subdivisions; ++i) {
            for (int j = 0; j <= subdivisions; ++j) {
                float u = (float)i / subdivisions - 0.5f;
                float v = (float)j / subdivisions - 0.5f;
                
                glm::vec3 pos = center + tangent * u + bitangent * v;
                vertices.emplace_back(pos.x, pos.y, pos.z, 1.0f);
                colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            }
        }
        
        // Generate indices for this face
        for (int i = 0; i < subdivisions; ++i) {
            for (int j = 0; j < subdivisions; ++j) {
                int topLeft = startVertex + i * (subdivisions + 1) + j;
                int topRight = topLeft + 1;
                int bottomLeft = topLeft + (subdivisions + 1);
                int bottomRight = bottomLeft + 1;
                
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
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
        vertices.emplace_back(0, 0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        // Base center
        vertices.emplace_back(0, -0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        // Base vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -0.5f, z, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        }
        
        // Side triangles
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

// Global variables
Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char activeAxis = 'X';
std::unique_ptr<model_t> currentModel;
HNode* currentNode = nullptr;

// Camera variables
float cameraDistance = 5.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
glm::mat4 modelRotation = glm::mat4(1.0f);

// Function declarations
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleModellingKeys(int key);
void handleInspectionKeys(int key);
void applyTransform(int direction);
void setupOpenGL();
void renderScene();
void updateCamera();
void printInstructions();

// Key handling implementation
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_M) {
        currentMode = MODELLING;
        std::cout << "Mode: MODELLING" << std::endl;
    }
    else if (key == GLFW_KEY_I) {
        currentMode = INSPECTION;
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

void handleModellingKeys(int key) {
    switch (key) {
        // Add shapes
        case GLFW_KEY_1:
            currentModel->addShape(std::make_unique<sphere_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Sphere added\n";
            break;
        case GLFW_KEY_2:
            currentModel->addShape(std::make_unique<cylinder_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cylinder added\n";
            break;
        case GLFW_KEY_3:
            currentModel->addShape(std::make_unique<box_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Box added\n";
            break;
        case GLFW_KEY_4:
            currentModel->addShape(std::make_unique<cone_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cone added\n";
            break;
        case GLFW_KEY_5:
            currentModel->removeLastShape();
            currentNode = currentModel->getLastNode();
            std::cout << "Last shape removed\n";
            break;

        // Transform mode selection
        case GLFW_KEY_R:
            transformMode = ROTATE;
            std::cout << "Transform mode: ROTATE\n";
            break;
        case GLFW_KEY_T:
            transformMode = TRANSLATE;
            std::cout << "Transform mode: TRANSLATE\n";
            break;
        case GLFW_KEY_G:
            transformMode = SCALE;
            std::cout << "Transform mode: SCALE\n";
            break;

        // Axis selection
        case GLFW_KEY_X:
            activeAxis = 'X';
            std::cout << "Active Axis: X\n";
            break;
        case GLFW_KEY_Y:
            activeAxis = 'Y';
            std::cout << "Active Axis: Y\n";
            break;
        case GLFW_KEY_Z:
            activeAxis = 'Z';
            std::cout << "Active Axis: Z\n";
            break;

        // Apply transformations
        case GLFW_KEY_KP_ADD:
        case GLFW_KEY_EQUAL:
            applyTransform(+1);
            break;
        case GLFW_KEY_KP_SUBTRACT:
        case GLFW_KEY_MINUS:
            applyTransform(-1);
            break;

        // Change color
        case GLFW_KEY_C: {
            float r, g, b;
            std::cout << "Enter RGB values (0-1): ";
            std::cin >> r >> g >> b;
            if (currentNode && currentNode->shape) {
                currentNode->shape->setColor(glm::vec4(r, g, b, 1.0f));
            }
            break;
        }
        
        // Save model
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
        // Load model
        case GLFW_KEY_L: {
            std::string filename;
            std::cout << "Enter filename to load: ";
            std::cin >> filename;
            if (currentModel->load(filename)) {
                currentNode = currentModel->getLastNode();
                // Reset camera to view loaded model
                cameraDistance = 5.0f;
                cameraAngleX = 0.0f;
                cameraAngleY = 0.0f;
                modelRotation = glm::mat4(1.0f);
            }
            break;
        }
        
        // Model rotation mode
        case GLFW_KEY_R:
            transformMode = ROTATE;
            std::cout << "Model rotation mode activated\n";
            break;
            
        // Axis selection for model rotation
        case GLFW_KEY_X:
            activeAxis = 'X';
            std::cout << "Model rotation axis: X\n";
            break;
        case GLFW_KEY_Y:
            activeAxis = 'Y';
            std::cout << "Model rotation axis: Y\n";
            break;
        case GLFW_KEY_Z:
            activeAxis = 'Z';
            std::cout << "Model rotation axis: Z\n";
            break;
            
        // Apply model rotation
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
            break