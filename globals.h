#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>


extern glm::mat4 projection;
extern glm::mat4 view;
extern GLuint shaderProgram;

enum Mode { MODELLING, INSPECTION };
enum TransformMode { NONE, ROTATE, TRANSLATE, SCALE };

extern Mode currentMode;
extern TransformMode transformMode;
extern char activeAxis;
struct model_node_t;
struct model_t; 
extern std::shared_ptr<model_t> currentModel;
extern std::shared_ptr<model_node_t> currentNode;

extern float cameraDistance, cameraAngleX, cameraAngleY;
extern glm::mat4 modelRotation;
