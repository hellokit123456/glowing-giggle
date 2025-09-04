#include <GL/glew.h>     
#include <GLFW/glfw3.h>  
#include <glm/glm.hpp>   
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "shape.h"
#include "globals.h"
#include "input.h"
#include "HIERARCHIAL.h"


bool Wireframe = false;
bool tesselationMode = false;
shape_t* getCurrentShape() {
    if (currentNode && currentNode->shape) {
        return currentNode->shape.get();  // unique_ptr -> raw pointer
    }
    return nullptr;
}

void applyTransform(int direction) {
    if (!currentNode) return;

    float step = 0.1f;
    float angle = glm::radians(5.0f);

    switch (transformMode) {
        case TRANSLATE:
            if (activeAxis == 'X') currentNode->translation = glm::translate(currentNode->translation, glm::vec3(direction * step, 0, 0));
            if (activeAxis == 'Y') currentNode->translation = glm::translate(currentNode->translation, glm::vec3(0, direction * step, 0));
            if (activeAxis == 'Z') currentNode->translation = glm::translate(currentNode->translation, glm::vec3(0, 0, direction * step));
            break;
        case ROTATE:
            if (activeAxis == 'X') currentNode->rotation = glm::rotate(currentNode->rotation, direction * angle, glm::vec3(1, 0, 0));
            if (activeAxis == 'Y') currentNode->rotation = glm::rotate(currentNode->rotation, direction * angle, glm::vec3(0, 1, 0));
            if (activeAxis == 'Z') currentNode->rotation = glm::rotate(currentNode->rotation, direction * angle, glm::vec3(0, 0, 1));
            break;
        case SCALE:
            if (activeAxis == 'X') currentNode->scale = glm::scale(currentNode->scale, glm::vec3(1 + direction * 0.1f, 1, 1));
            if (activeAxis == 'Y') currentNode->scale = glm::scale(currentNode->scale, glm::vec3(1, 1 + direction * 0.1f, 1));
            if (activeAxis == 'Z') currentNode->scale = glm::scale(currentNode->scale, glm::vec3(1, 1, 1 + direction * 0.1f));
            break;
        default:
            break;
    }
}
void setupOpenGL();
void renderScene(GLuint shaderProgram);
void updateCamera();
void printInstructions();
    shape_t* shape = getCurrentShape(); 

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
    else if (key == GLFW_KEY_W ){
    Wireframe = !Wireframe;
    
   if (Wireframe){
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);}
   else
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   }}
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
        

        case GLFW_KEY_U: // Move UP to parent
            if (currentNode->parent.lock()) {
                currentNode = currentNode->parent.lock();
                std::cout << "Selected parent node.\n";
            } else {
                std::cout << "Already at the root node.\n";
            }
            break;
        case GLFW_KEY_J: // Move DOWN to first child
            if (!currentNode->children.empty()) {
                currentNode = currentNode->children.front();
                std::cout << "Selected first child node.\n";
            } else {
                std::cout << "Selected node has no children.\n";
            }
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
        //Tesselation implementation
         case GLFW_KEY_A:
            tesselationMode = !tesselationMode;
            if (tesselationMode) {
                std::cout << "TESSELLATION MODE ACTIVATED " << std::endl;
                std::cout << "Press number keys 1-6 to set tessellation level" << std::endl;
                std::cout << "Press A again to exit tessellation mode" << std::endl;
                if (currentNode && currentNode->shape) {
                    std::cout << "Current tessellation level: " << currentNode->shape->getLevel() << std::endl;
                    std::cout << "Current triangle count: " << currentNode->shape->indices.size() / 3 << std::endl;
                } else {
                    std::cout << "No shape selected!" << std::endl;
                }
            } else {
                std::cout << "TESSELLATION MODE DEACTIVATED " << std::endl;
            }
            break;
        
    // Add shapes
        case GLFW_KEY_1:
          if (tesselationMode && currentNode && currentNode->shape) {
                currentNode->shape->setLevel(1);
            } else if (!tesselationMode) {
            currentModel->addShape(std::make_unique<sphere_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Sphere added\n";}
            break;
        case GLFW_KEY_2:
          if (tesselationMode && currentNode && currentNode->shape) {
                currentNode->shape->setLevel(2);
            } else if (!tesselationMode) {
            currentModel->addShape(std::make_unique<cylinder_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cylinder added\n";}
            break;
        case GLFW_KEY_3:
         if (tesselationMode && currentNode && currentNode->shape) {
                currentNode->shape->setLevel(3);
            } else if (!tesselationMode) {
            currentModel->addShape(std::make_unique<box_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Box added\n";}
            break;
        case GLFW_KEY_4:
          if (tesselationMode && currentNode && currentNode->shape) {
                currentNode->shape->setLevel(4);
            } else if (!tesselationMode) {
            currentModel->addShape(std::make_unique<cone_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cone added\n";}
            break;
        case GLFW_KEY_5:
         if (tesselationMode && currentNode && currentNode->shape) {
                currentNode->shape->setLevel(5);
            } else if (!tesselationMode) {
            currentModel->removeLastShape();
            currentNode = currentModel->getLastNode();
            std::cout << "Last shape removed\n";}
            break;
        case GLFW_KEY_6:
            if (tesselationMode && currentNode && currentNode->shape) {
                currentNode->shape->setLevel(6);
            }
            break;   
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
            break;}}
