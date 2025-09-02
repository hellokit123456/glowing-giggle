#ifndef HIERARCHICAL_H
#define HIERARCHICAL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <memory>
#include <vector>
#include <string>
#include "shape.h"

// Shader program (declared in main.cpp)
extern GLuint shaderProgram;

// The single, unified node class for the scene hierarchy
struct model_node_t : public std::enable_shared_from_this<model_node_t> {
    inline static int next_id = 0;
    int id;
    std::shared_ptr<shape_t> shape; // Owns the shape data
    ShapeType type;

    // Transformations
    glm::mat4 translation{1.0f};
    glm::mat4 rotation{1.0f};
    glm::mat4 scale{1.0f};
    
    // Hierarchy
    std::weak_ptr<model_node_t> parent;
    std::vector<std::shared_ptr<model_node_t>> children;
    
    // Properties
    glm::vec4 color{1.0f};

    model_node_t(std::shared_ptr<shape_t> s = nullptr, ShapeType t = SPHERE_SHAPE);
    void addChild(const std::shared_ptr<model_node_t>& child);
    glm::mat4 getTransform() const;
};

// Main model class containing the scene hierarchy
class model_t {
private:
    std::vector<std::shared_ptr<model_node_t>> shapes; // A flat list for easy access
    int next_id = 0;
    std::shared_ptr<model_node_t> findMNodeById(int id);

public:
    std::shared_ptr<model_node_t> root_node; // The single root of the scene

    model_t();
    std::shared_ptr<model_node_t> getRoot();
    const std::vector<std::shared_ptr<model_node_t>>& getShapes() const;
    void addShape(std::unique_ptr<shape_t> shape);
    void addShapeToParent(int parent_ui_id, std::unique_ptr<shape_t> shape);
    void removeLastShape();
    std::shared_ptr<model_node_t> getCurrentShape();
    std::shared_ptr<model_node_t> getLastNode();
    void rotateModel(char axis, bool positive);
    void render(); // Note: render() is defined in HIERARCHIAL_NODE.cpp but not used by main.cpp
    size_t getShapeCount() const;
    void clear();
    void save(const std::string& filename);
    bool load(const std::string& filename);
};

#endif