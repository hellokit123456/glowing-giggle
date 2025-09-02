#ifndef INPUT_H
#define INPUT_H

#include <GLFW/glfw3.h>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleModellingKeys(int key);
void handleInspectionKeys(int key);
void applyTransform(int direction);

#endif