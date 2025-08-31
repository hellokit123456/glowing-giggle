#include <GLFW/glfw3.h>
#include <iostream>
enum Mode { MODELLING, INSPECTION };
enum TransformMode { NONE, ROTATE, TRANSLATE, SCALE };

Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char activeAxis = 'X';   // default axis
model_t* currentModel = nullptr; 
shape_t* currentShape = nullptr; // last added shape
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
    else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        io::saveModel(*currentModel, "scene.mod");
        }

    if (currentMode == MODELLING) {
        handleModellingKeys(key);
    }
}
void handleModellingKeys(int key) {
    switch (key) {
        // Add shapes
       
        
        case GLFW_KEY_1:
            currentShape = new sphere_t(1); // default tessellation
            currentModel->addShape(currentShape);
            std::cout << "Sphere added\n";
            break;
        case GLFW_KEY_2:
            currentShape = new cylinder_t(1);
            currentModel->addShape(currentShape);
            std::cout << "Cylinder added\n";
            break;
        case GLFW_KEY_3:
            currentShape = new box_t(1);
            currentModel->addShape(currentShape);
            std::cout << "Box added\n";
            break;
        case GLFW_KEY_4:
            currentShape = new cone_t(1);
            currentModel->addShape(currentShape);
            std::cout << "Cone added\n";
            break;
         

        // Remove last shape
        case GLFW_KEY_5:
            currentModel->removeLastShape();
            currentShape = nullptr;
            std::cout << "Last shape removed\n";
            break;
        case GLFW_KEY_6:
            currentShape = new hemisphere_t(1);
            currentModel->addShape(currentShape);
            std::cout << "Hemisphere added\n";
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

        // Apply + or - transformations
        case GLFW_KEY_KP_ADD: // '+' key
        case GLFW_KEY_EQUAL:      // '=' works as '+'
            applyTransform(+1);
            break;
        case GLFW_KEY_KP_SUBTRACT: // '-' key
        case GLFW_KEY_MINUS:
            applyTransform(-1);
            break;

        // Change color
        case GLFW_KEY_C: {
            float r, g, b;
            std::cout << "Enter RGB values (0-1): ";
            std::cin >> r >> g >> b;
            if (currentShape)
                currentShape->setColor(glm::vec4(r, g, b, 1.0f));
            break;
        }
    }
}
void applyTransform(int direction) {
    if (!currentShape) return;

    float step = 0.1f * direction;  
    float angle = 5.0f * direction; // degrees

    switch (transformMode) {
        case ROTATE:
            currentShape->rotate(activeAxis, glm::radians(angle));
            break;
        case TRANSLATE:
            currentShape->translate(activeAxis, step);
            break;
        case SCALE:
            currentShape->scale(activeAxis, 1.0f + 0.1f * direction);
            break;
        default: break;
    }
}

