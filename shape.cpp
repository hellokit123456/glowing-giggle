#include "shape.h"
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

const float PI = 3.14159265359f;
const float ROTATION_STEP = 15.0f; // degrees
const float TRANSLATION_STEP = 0.1f;
const float SCALE_STEP = 0.1f;

// Sphere implementation
void sphere_t::generateSphere() {
    vertices.clear();
    colors.clear();
    
    // Calculate subdivision based on level
    int latitudeBands = 4 + level * 4;   // 4, 8, 12, 16, 20
    int longitudeBands = 6 + level * 6;  // 6, 12, 18, 24, 30
    
    // Generate vertices
    for (int lat = 0; lat <= latitudeBands; ++lat) {
        float theta = lat * PI / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= longitudeBands; ++lon) {
            float phi = lon * 2 * PI / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(glm::vec4(x * radius, y * radius, z * radius, 1.0f));
            colors.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Default red
        }
    }
    
    // Generate triangles (indices would be used in actual rendering)
    // For simplicity, storing as triangle vertices
    std::vector<glm::vec4> triangleVertices;
    std::vector<glm::vec4> triangleColors;
    
    for (int lat = 0; lat < latitudeBands; ++lat) {
        for (int lon = 0; lon < longitudeBands; ++lon) {
            int first = lat * (longitudeBands + 1) + lon;
            int second = first + longitudeBands + 1;
            
            // First triangle
            triangleVertices.push_back(vertices[first]);
            triangleVertices.push_back(vertices[second]);
            triangleVertices.push_back(vertices[first + 1]);
            
            triangleColors.push_back(colors[first]);
            triangleColors.push_back(colors[second]);
            triangleColors.push_back(colors[first + 1]);
            
            // Second triangle
            triangleVertices.push_back(vertices[second]);
            triangleVertices.push_back(vertices[second + 1]);
            triangleVertices.push_back(vertices[first + 1]);
            
            triangleColors.push_back(colors[second]);
            triangleColors.push_back(colors[second + 1]);
            triangleColors.push_back(colors[first + 1]);
        }
    }
    
    vertices = triangleVertices;
    colors = triangleColors;
}

void sphere_t::draw() {
    glPushMatrix();
    glMultMatrixf(&transform[0][0]);
    
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < vertices.size(); ++i) {
        glColor4f(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    glEnd();
    
    glPopMatrix();
}

void sphere_t::rotate(char axis, bool positive) {
    float angle = positive ? ROTATION_STEP : -ROTATION_STEP;
    glm::vec3 axisVec;
    
    switch (axis) {
        case 'X': axisVec = glm::vec3(1, 0, 0); break;
        case 'Y': axisVec = glm::vec3(0, 1, 0); break;
        case 'Z': axisVec = glm::vec3(0, 0, 1); break;
    }
    
    transform = glm::rotate(transform, glm::radians(angle), axisVec);
}

void sphere_t::translate(char axis, bool positive) {
    float distance = positive ? TRANSLATION_STEP : -TRANSLATION_STEP;
    glm::vec3 translation(0.0f);
    
    switch (axis) {
        case 'X': translation.x = distance; break;
        case 'Y': translation.y = distance; break;
        case 'Z': translation.z = distance; break;
    }
    
    transform = glm::translate(transform, translation);
}

void sphere_t::scale(char axis, bool positive) {
    float factor = positive ? (1.0f + SCALE_STEP) : (1.0f - SCALE_STEP);
    glm::vec3 scaleVec(1.0f);
    
    switch (axis) {
        case 'X': scaleVec.x = factor; break;
        case 'Y': scaleVec.y = factor; break;
        case 'Z': scaleVec.z = factor; break;
    }
    
    transform = glm::scale(transform, scaleVec);
}

// Cylinder implementation
void cylinder_t::generateCylinder() {
    vertices.clear();
    colors.clear();
    
    int segments = 6 + level * 6; // 6, 12, 18, 24, 30
    int heightSegments = 2 + level; // 2, 3, 4, 5, 6
    
    std::vector<glm::vec4> triangleVertices;
    std::vector<glm::vec4> triangleColors;
    
    // Generate side vertices
    for (int h = 0; h <= heightSegments; ++h) {
        float y = (h / (float)heightSegments - 0.5f) * height;
        
        for (int s = 0; s <= segments; ++s) {
            float angle = s * 2 * PI / segments;
            float x = cos(angle) * radius;
            float z = sin(angle) * radius;
            
            vertices.push_back(glm::vec4(x, y, z, 1.0f));
            colors.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // Default green
        }
    }
    
    // Generate side triangles
    for (int h = 0; h < heightSegments; ++h) {
        for (int s = 0; s < segments; ++s) {
            int current = h * (segments + 1) + s;
            int next = current + segments + 1;
            
            // Triangle 1
            triangleVertices.push_back(vertices[current]);
            triangleVertices.push_back(vertices[next]);
            triangleVertices.push_back(vertices[current + 1]);
            
            // Triangle 2
            triangleVertices.push_back(vertices[next]);
            triangleVertices.push_back(vertices[next + 1]);
            triangleVertices.push_back(vertices[current + 1]);
            
            for (int i = 0; i < 6; ++i) {
                triangleColors.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
            }
        }
    }
    
    // Add top and bottom caps
    glm::vec4 topCenter(0, height/2, 0, 1);
    glm::vec4 bottomCenter(0, -height/2, 0, 1);
    
    for (int s = 0; s < segments; ++s) {
        float angle1 = s * 2 * PI / segments;
        float angle2 = (s + 1) * 2 * PI / segments;
        
        // Top cap
        triangleVertices.push_back(topCenter);
        triangleVertices.push_back(glm::vec4(cos(angle1) * radius, height/2, sin(angle1) * radius, 1));
        triangleVertices.push_back(glm::vec4(cos(angle2) * radius, height/2, sin(angle2) * radius, 1));
        
        // Bottom cap
        triangleVertices.push_back(bottomCenter);
        triangleVertices.push_back(glm::vec4(cos(angle2) * radius, -height/2, sin(angle2) * radius, 1));
        triangleVertices.push_back(glm::vec4(cos(angle1) * radius, -height/2, sin(angle1) * radius, 1));
        
        for (int i = 0; i < 6; ++i) {
            triangleColors.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        }
    }
    
    vertices = triangleVertices;
    colors = triangleColors;
}

void cylinder_t::draw() {
    glPushMatrix();
    glMultMatrixf(&transform[0][0]);
    
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < vertices.size(); ++i) {
        glColor4f(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    glEnd();
    
    glPopMatrix();
}

void cylinder_t::rotate(char axis, bool positive) {
    float angle = positive ? ROTATION_STEP : -ROTATION_STEP;
    glm::vec3 axisVec;
    
    switch (axis) {
        case 'X': axisVec = glm::vec3(1, 0, 0); break;
        case 'Y': axisVec = glm::vec3(0, 1, 0); break;
        case 'Z': axisVec = glm::vec3(0, 0, 1); break;
    }
    
    transform = glm::rotate(transform, glm::radians(angle), axisVec);
}

void cylinder_t::translate(char axis, bool positive) {
    float distance = positive ? TRANSLATION_STEP : -TRANSLATION_STEP;
    glm::vec3 translation(0.0f);
    
    switch (axis) {
        case 'X': translation.x = distance; break;
        case 'Y': translation.y = distance; break;
        case 'Z': translation.z = distance; break;
    }
    
    transform = glm::translate(transform, translation);
}

void cylinder_t::scale(char axis, bool positive) {
    float factor = positive ? (1.0f + SCALE_STEP) : (1.0f - SCALE_STEP);
    glm::vec3 scaleVec(1.0f);
    
    switch (axis) {
        case 'X': scaleVec.x = factor; break;
        case 'Y': scaleVec.y = factor; break;
        case 'Z': scaleVec.z = factor; break;
    }
    
    transform = glm::scale(transform, scaleVec);
}

// Box implementation
void box_t::generateBox() {
    vertices.clear();
    colors.clear();
    
    float hw = width / 2.0f;
    float hh = height / 2.0f;
    float hd = depth / 2.0f;
    
    // Subdivision based on level
    int subdivisions = level + 1; // 1, 2, 3, 4, 5
    
    std::vector<glm::vec4> triangleVertices;
    std::vector<glm::vec4> triangleColors;
    
    // Generate each face with subdivisions
    // Front face (z = hd)
    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {
            float x1 = -hw + (2.0f * hw * i) / subdivisions;
            float x2 = -hw + (2.0f * hw * (i + 1)) / subdivisions;
            float y1 = -hh + (2.0f * hh * j) / subdivisions;
            float y2 = -hh + (2.0f * hh * (j + 1)) / subdivisions;
            
            // Triangle 1
            triangleVertices.push_back(glm::vec4(x1, y1, hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x2, y1, hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x1, y2, hd, 1.0f));
            
            // Triangle 2
            triangleVertices.push_back(glm::vec4(x2, y1, hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x2, y2, hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x1, y2, hd, 1.0f));
            
            for (int k = 0; k < 6; ++k) {
                triangleColors.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)); // Blue
            }
        }
    }
    
    // Back face (z = -hd)
    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {
            float x1 = -hw + (2.0f * hw * i) / subdivisions;
            float x2 = -hw + (2.0f * hw * (i + 1)) / subdivisions;
            float y1 = -hh + (2.0f * hh * j) / subdivisions;
            float y2 = -hh + (2.0f * hh * (j + 1)) / subdivisions;
            
            // Triangle 1 (reversed winding)
            triangleVertices.push_back(glm::vec4(x1, y1, -hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x1, y2, -hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x2, y1, -hd, 1.0f));
            
            // Triangle 2 (reversed winding)
            triangleVertices.push_back(glm::vec4(x2, y1, -hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x1, y2, -hd, 1.0f));
            triangleVertices.push_back(glm::vec4(x2, y2, -hd, 1.0f));
            
            for (int k = 0; k < 6; ++k) {
                triangleColors.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
            }
        }
    }
    
    // Similar subdivision for other 4 faces...
    // Left, Right, Top, Bottom faces would follow similar pattern
    
    vertices = triangleVertices;
    colors = triangleColors;
}

void box_t::draw() {
    glPushMatrix();
    glMultMatrixf(&transform[0][0]);
    
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < vertices.size(); ++i) {
        glColor4f(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    glEnd();
    
    glPopMatrix();
}

void box_t::rotate(char axis, bool positive) {
    float angle = positive ? ROTATION_STEP : -ROTATION_STEP;
    glm::vec3 axisVec;
    
    switch (axis) {
        case 'X': axisVec = glm::vec3(1, 0, 0); break;
        case 'Y': axisVec = glm::vec3(0, 1, 0); break;
        case 'Z': axisVec = glm::vec3(0, 0, 1); break;
    }
    
    transform = glm::rotate(transform, glm::radians(angle), axisVec);
}

void box_t::translate(char axis, bool positive) {
    float distance = positive ? TRANSLATION_STEP : -TRANSLATION_STEP;
    glm::vec3 translation(0.0f);
    
    switch (axis) {
        case 'X': translation.x = distance; break;
        case 'Y': translation.y = distance; break;
        case 'Z': translation.z = distance; break;
    }
    
    transform = glm::translate(transform, translation);
}

void box_t::scale(char axis, bool positive) {
    float factor = positive ? (1.0f + SCALE_STEP) : (1.0f - SCALE_STEP);
    glm::vec3 scaleVec(1.0f);
    
    switch (axis) {
        case 'X': scaleVec.x = factor; break;
        case 'Y': scaleVec.y = factor; break;
        case 'Z': scaleVec.z = factor; break;
    }
    
    transform = glm::scale(transform, scaleVec);
}

// Cone implementation
void cone_t::generateCone() {
    vertices.clear();
    colors.clear();
    
    int segments = 6 + level * 6; // 6, 12, 18, 24, 30
    int heightSegments = 2 + level; // 2, 3, 4, 5, 6
    
    std::vector<glm::vec4> triangleVertices;
    std::vector<glm::vec4> triangleColors;
    
    glm::vec4 apex(0.0f, height/2, 0.0f, 1.0f);
    glm::vec4 baseCenter(0.0f, -height/2, 0.0f, 1.0f);
    
    // Generate side triangles
    for (int s = 0; s < segments; ++s) {
        float angle1 = s * 2 * PI / segments;
        float angle2 = (s + 1) * 2 * PI / segments;
        
        glm::vec4 base1(cos(angle1) * radius, -height/2, sin(angle1) * radius, 1.0f);
        glm::vec4 base2(cos(angle2) * radius, -height/2, sin(angle2) * radius, 1.0f);
        
        // Side triangle
        triangleVertices.push_back(apex);
        triangleVertices.push_back(base1);
        triangleVertices.push_back(base2);
        
        // Base triangle
        triangleVertices.push_back(baseCenter);
        triangleVertices.push_back(base2);
        triangleVertices.push_back(base1);
        
        for (int i = 0; i < 6; ++i) {
            triangleColors.push_back(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
        }
    }
    
    vertices = triangleVertices;
    colors = triangleColors;
}

void cone_t::draw() {
    glPushMatrix();
    glMultMatrixf(&transform[0][0]);
    
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < vertices.size(); ++i) {
        glColor4f(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    glEnd();
    
    glPopMatrix();
}

void cone_t::rotate(char axis, bool positive) {
    float angle = positive ? ROTATION_STEP : -ROTATION_STEP;
    glm::vec3 axisVec;
    
    switch (axis) {
        case 'X': axisVec = glm::vec3(1, 0, 0); break;
        case 'Y': axisVec = glm::vec3(0, 1, 0); break;
        case 'Z': axisVec = glm::vec3(0, 0, 1); break;
    }
    
    transform = glm::rotate(transform, glm::radians(angle), axisVec);
}

void cone_t::translate(char axis, bool positive) {
    float distance = positive ? TRANSLATION_STEP : -TRANSLATION_STEP;
    glm::vec3 translation(0.0f);
    
    switch (axis) {
        case 'X': translation.x = distance; break;
        case 'Y': translation.y = distance; break;
        case 'Z': translation.z = distance; break;
    }
    
    transform = glm::translate(transform, translation);
}

void cone_t::scale(char axis, bool positive) {
    float factor = positive ? (1.0f + SCALE_STEP) : (1.0f - SCALE_STEP);
    glm::vec3 scaleVec(1.0f);
    
    switch (axis) {
        case 'X': scaleVec.x = factor; break;
        case 'Y': scaleVec.y = factor; break;
        case 'Z': scaleVec.z = factor; break;
    }
    
    transform = glm::scale(transform, scaleVec);
}

// Model implementation
void model_t::addShape(std::shared_ptr<shape_t> shape) {
    auto node = std::make_shared<model_node_t>();
    node->shape = shape;
    
    root->addChild(node);
    shapes.push_back(node);
}

void model_t::removeLastShape() {
    if (shapes.empty()) return;
    
    auto lastShape = shapes.back();
    shapes.pop_back();
    
    // Remove from parent's children
    if (lastShape->parent) {
        auto& children = lastShape->parent->children;
        children.erase(std::remove(children.begin(), children.end(), lastShape), children.end());
    }
}

void model_t::clear() {
    shapes.clear();
    root->children.clear();
}

void model_t::draw() {
    std::function<void(std::shared_ptr<model_node_t>)> drawNode = 
        [&](std::shared_ptr<model_node_t> node) {
            if (node->shape) {
                glPushMatrix();
                glm::mat4 transform = node->getTransform();
                glMultMatrixf(&transform[0][0]);
                node->shape->draw();
                glPopMatrix();
            }
            
            for (auto& child : node->children) {
                drawNode(child);
            }
        };
    
    drawNode(root);
}

glm::vec3 model_t::getModelCentroid() const {
    if (shapes.empty()) return glm::vec3(0.0f);
    
    glm::vec3 centroid(0.0f);
    for (const auto& node : shapes) {
        centroid += node->getCentroid();
    }
    return centroid / static_cast<float>(shapes.size());
}

void model_t::rotateModel(char axis, bool positive) {
    float angle = positive ? ROTATION_STEP : -ROTATION_STEP;
    glm::vec3 axisVec;
    
    switch (axis) {
        case 'X': axisVec = glm::vec3(1, 0, 0); break;
        case 'Y': axisVec = glm::vec3(0, 1, 0); break;
        case 'Z': axisVec = glm::vec3(0, 0, 1); break;
    }
    
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axisVec);
    
    // Apply rotation to root node
    root->rotation = rotationMatrix * root->rotation;
}();
}

void model_t::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for reading");
    }
    
    clear(); // Remove existing model
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "shapes") {
            int shapeCount;
            iss >> shapeCount;
            // Prepare for loading shapes
        } else if (command == "shape_type") {
            int typeInt;
            iss >> typeInt;
            ShapeType type = static_cast<ShapeType>(typeInt);
            
            // Read level
            std::getline(file, line);
            std::istringstream levelStream(line);
            std::string levelCmd;
            unsigned int level;
            levelStream >> levelCmd >> level;
            
            // Create shape
            std::shared_ptr<shape_t> newShape;
            switch (type) {
                case SPHERE_SHAPE:
                    newShape = std::make_shared<sphere_t>(level);
                    break;
                case CYLINDER_SHAPE:
                    newShape = std::make_shared<cylinder_t>(level);
                    break;
                case BOX_SHAPE:
                    newShape = std::make_shared<box_t>(level);
                    break;
                case CONE_SHAPE:
                    newShape = std::make_shared<cone_t>(level);
                    break;
            }
            
            addShape(newShape);
            auto currentNode = shapes.back();
            
            // Read transforms
            for (int i = 0; i < 3; ++i) {
                std::getline(file, line);
                std::istringstream transformStream(line);
                std::string transformType;
                transformStream >> transformType;
                
                glm::mat4 matrix;
                for (int row = 0; row < 4; ++row) {
                    for (int col = 0; col < 4; ++col) {
                        transformStream >> matrix[row][col];
                    }
                }
                
                if (transformType == "rotation") {
                    currentNode->rotation = matrix;
                } else if (transformType == "scale") {
                    currentNode->scale = matrix;
                } else if (transformType == "translation") {
                    currentNode->translation = matrix;
                }
            }
            
            // Read color
            std::getline(file, line);
            std::istringstream colorStream(line);
            std::string colorCmd;
            float r, g, b, a;
            colorStream >> colorCmd >> r >> g >> b >> a;
            newShape->setColor(glm::vec4(r, g, b, a));
        }
    }
    
    file.close();
}

glm::vec3 model_t::getModelCentroid() const {
    if (shapes.empty()) return glm::vec3(0.0f);
    
    glm::vec3 centroid(0.0f);
    for (const auto& node : shapes) {
        centroid += node->getCentroid();
    }
    return centroid / static_cast<float>(shapes.size());
}

void model_t::rotateModel(char axis, bool positive) {
    float angle = positive ? ROTATION_STEP : -ROTATION_STEP;
    glm::vec3 axisVec;
    
    switch (axis) {
        case 'X': axisVec = glm::vec3(1, 0, 0); break;
        case 'Y': axisVec = glm::vec3(0, 1, 0); break;
        case 'Z': axisVec = glm::vec3(0, 0, 1); break;
    }
    
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axisVec);
    
    // Apply rotation to root node
    root->rotation = rotationMatrix * root->rotation;
}
