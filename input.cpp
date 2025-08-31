#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <memory>
#include "shape.h"

// Global variables for application state
enum Mode {
    MODELLING,
    INSPECTION
};

enum TransformMode {
    NONE,
    ROTATION,
    TRANSLATION,
    SCALING
};

// Application state
Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char currentAxis = 'X';
std::unique_ptr<model_t> currentModel;
GLFWwindow* window;

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Function declarations
void initializeOpenGL();
void setupCallbacks();
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleModellingMode(int key);
void handleInspectionMode(int key);
void handleTransformMode(int key);
void render();
void cleanup();
void printCurrentMode();
void addShape(ShapeType type);
void removeLastShape();
void activateRotationMode();
void activateTranslationMode();
void activateScalingMode();
void chooseAxis(char axis);
void applyTransformation(bool positive);
void changeShapeColor();
void saveModel();
void loadModel();

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create window
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "3D Shape Modeler", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Initialize OpenGL settings
    initializeOpenGL();
    
    // Setup input callbacks
    setupCallbacks();
    
    // Initialize model
    currentModel = std::make_unique<model_t>();
    
    // Print initial mode
    printCurrentMode();

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Render
        render();
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    cleanup();
    return 0;
}

void initializeOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Setup basic lighting and projection matrices here
    // This would typically involve shader setup, but that's beyond current scope
}

void setupCallbacks() {
    glfwSetKeyCallback(window, keyCallback);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    
    // Global keys
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    
    if (key == GLFW_KEY_M) {
        currentMode = MODELLING;
        transformMode = NONE;
        printCurrentMode();
        return;
    }
    
    if (key == GLFW_KEY_I) {
        currentMode = INSPECTION;
        transformMode = NONE;
        printCurrentMode();
        return;
    }
    
    // Handle transform mode keys first (common to both modes)
    if (transformMode != NONE) {
        handleTransformMode(key);
        return;
    }
    
    // Mode-specific handling
    if (currentMode == MODELLING) {
        handleModellingMode(key);
    } else if (currentMode == INSPECTION) {
        handleInspectionMode(key);
    }
}

void handleModellingMode(int key) {
    switch (key) {
        case GLFW_KEY_1:
            addShape(SPHERE_SHAPE);
            break;
        case GLFW_KEY_2:
            addShape(CYLINDER_SHAPE);
            break;
        case GLFW_KEY_3:
            addShape(BOX_SHAPE);
            break;
        case GLFW_KEY_4:
            addShape(CONE_SHAPE);
            break;
        case GLFW_KEY_5:
            removeLastShape();
            break;
        case GLFW_KEY_R:
            activateRotationMode();
            break;
        case GLFW_KEY_T:
            activateTranslationMode();
            break;
        case GLFW_KEY_G:
            activateScalingMode();
            break;
        case GLFW_KEY_C:
            changeShapeColor();
            break;
        case GLFW_KEY_S:
            saveModel();
            break;
    }
}

void handleInspectionMode(int key) {
    switch (key) {
        case GLFW_KEY_L:
            loadModel();
            break;
        case GLFW_KEY_R:
            activateRotationMode();
            break;
    }
}

void handleTransformMode(int key) {
    switch (key) {
        case GLFW_KEY_X:
            chooseAxis('X');
            break;
        case GLFW_KEY_Y:
            chooseAxis('Y');
            break;
        case GLFW_KEY_Z:
            chooseAxis('Z');
            break;
        case GLFW_KEY_EQUAL: // '+' key
            applyTransformation(true);
            break;
        case GLFW_KEY_MINUS: // '-' key
            applyTransformation(false);
            break;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_SPACE:
            transformMode = NONE;
            std::cout << "Transform mode deactivated" << std::endl;
            break;
    }
}

void printCurrentMode() {
    if (currentMode == MODELLING) {
        std::cout << "Current Mode: MODELLING" << std::endl;
        std::cout << "Controls: 1-4 (add shapes), 5 (remove), R (rotate), T (translate), G (scale), C (color), S (save)" << std::endl;
    } else {
        std::cout << "Current Mode: INSPECTION" << std::endl;
        std::cout << "Controls: L (load), R (rotate model)" << std::endl;
    }
}

void addShape(ShapeType type) {
    if (!currentModel) return;
    
    std::shared_ptr<shape_t> newShape;
    unsigned int defaultLevel = 2; // Medium tesselation
    
    switch (type) {
        case SPHERE_SHAPE:
            newShape = std::make_shared<sphere_t>(defaultLevel);
            std::cout << "Added sphere" << std::endl;
            break;
        case CYLINDER_SHAPE:
            newShape = std::make_shared<cylinder_t>(defaultLevel);
            std::cout << "Added cylinder" << std::endl;
            break;
        case BOX_SHAPE:
            newShape = std::make_shared<box_t>(defaultLevel);
            std::cout << "Added box" << std::endl;
            break;
        case CONE_SHAPE:
            newShape = std::make_shared<cone_t>(defaultLevel);
            std::cout << "Added cone" << std::endl;
            break;
    }
    
    currentModel->addShape(newShape);
}

void removeLastShape() {
    if (!currentModel) return;
    
    currentModel->removeLastShape();
    std::cout << "Removed last shape" << std::endl;
}

void activateRotationMode() {
    transformMode = ROTATION;
    currentAxis = 'X'; // Default axis
    std::cout << "Rotation mode activated. Choose axis (X/Y/Z), then use +/- to rotate" << std::endl;
}

void activateTranslationMode() {
    if (currentMode != MODELLING) return;
    
    transformMode = TRANSLATION;
    currentAxis = 'X'; // Default axis
    std::cout << "Translation mode activated. Choose axis (X/Y/Z), then use +/- to translate" << std::endl;
}

void activateScalingMode() {
    if (currentMode != MODELLING) return;
    
    transformMode = SCALING;
    currentAxis = 'X'; // Default axis
    std::cout << "Scaling mode activated. Choose axis (X/Y/Z), then use +/- to scale" << std::endl;
}

void chooseAxis(char axis) {
    currentAxis = axis;
    std::cout << "Selected axis: " << axis << std::endl;
}

void applyTransformation(bool positive) {
    if (!currentModel) return;
    
    if (currentMode == MODELLING) {
        auto currentShape = currentModel->getCurrentShape();
        if (!currentShape || !currentShape->shape) {
            std::cout << "No shape selected" << std::endl;
            return;
        }
        
        switch (transformMode) {
            case ROTATION:
                currentShape->shape->rotate(currentAxis, positive);
                std::cout << "Rotated shape around " << currentAxis << " axis" << std::endl;
                break;
            case TRANSLATION:
                currentShape->shape->translate(currentAxis, positive);
                std::cout << "Translated shape along " << currentAxis << " axis" << std::endl;
                break;
            case SCALING:
                currentShape->shape->scale(currentAxis, positive);
                std::cout << "Scaled shape along " << currentAxis << " axis" << std::endl;
                break;
            default:
                break;
        }
    } else if (currentMode == INSPECTION && transformMode == ROTATION) {
        currentModel->rotateModel(currentAxis, positive);
        std::cout << "Rotated entire model around " << currentAxis << " axis" << std::endl;
    }
}

void changeShapeColor() {
    if (currentMode != MODELLING || !currentModel) return;
    
    auto currentShape = currentModel->getCurrentShape();
    if (!currentShape || !currentShape->shape) {
        std::cout << "No shape selected" << std::endl;
        return;
    }
    
    float r, g, b;
    std::cout << "Enter RGB color values (0.0 - 1.0):" << std::endl;
    std::cout << "Red: ";
    std::cin >> r;
    std::cout << "Green: ";
    std::cin >> g;
    std::cout << "Blue: ";
    std::cin >> b;
    
    // Clamp values
    r = std::max(0.0f, std::min(1.0f, r));
    g = std::max(0.0f, std::min(1.0f, g));
    b = std::max(0.0f, std::min(1.0f, b));
    
    currentShape->shape->setColor(glm::vec4(r, g, b, 1.0f));
    std::cout << "Color changed to RGB(" << r << ", " << g << ", " << b << ")" << std::endl;
}

void saveModel() {
    if (currentMode != MODELLING || !currentModel) return;
    
    std::string filename;
    std::cout << "Enter filename (without extension): ";
    std::cin >> filename;
    filename += ".mod";
    
    try {
        currentModel->saveToFile(filename);
        std::cout << "Model saved to " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving model: " << e.what() << std::endl;
    }
}

void loadModel() {
    if (currentMode != INSPECTION || !currentModel) return;
    
    std::string filename;
    std::cout << "Enter model filename: ";
    std::cin >> filename;
    
    try {
        currentModel->loadFromFile(filename);
        std::cout << "Model loaded from " << filename << std::endl;
        
        // Position camera to view entire model
        glm::vec3 modelCentroid = currentModel->getModelCentroid();
        // Here you would adjust camera position based on modelCentroid
        // This requires your camera/view matrix setup
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (currentModel) {
        currentModel->draw();
    }
    
    // Additional rendering code would go here
    // (setting up view/projection matrices, shader uniforms, etc.)
}

void cleanup() {
    if (currentModel) {
        currentModel->clear();
        currentModel.reset();
    }
    
    glfwTerminate();
    std::cout << "Application cleaned up successfully" << std::endl;
}
