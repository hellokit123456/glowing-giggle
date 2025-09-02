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

class HNode; // Forward declaration for model_node_t

// UI-facing node that mirrors an HNode
struct model_node_t : public std::enable_shared_from_this<model_node_t> {
    HNode* hnode_ptr;
    inline static int next_id = 0;
    int id;
    glm::mat4 translation{1.0f};
    glm::mat4 rotation{1.0f};
    glm::mat4 scale{1.0f};
    std::shared_ptr<shape_t> shape;
    ShapeType type;
    glm::vec4 color{1.0f};
    std::weak_ptr<model_node_t> parent;
    std::vector<std::shared_ptr<model_node_t>> children;

    model_node_t(std::shared_ptr<shape_t> s = nullptr, ShapeType t = SPHERE_SHAPE);
    void addChild(const std::shared_ptr<model_node_t>& child);
    glm::mat4 getTransform() const;
};

// Main model class containing the scene hierarchy
class model_t {
private:
    std::unique_ptr<HNode> root_hnode;
    std::vector<std::shared_ptr<model_node_t>> shapes;
    int next_id = 0;
    std::shared_ptr<model_node_t> findMNodeById(int id);
    bool removeHChildFromParent(HNode* parent, HNode* target);

public:
    std::shared_ptr<model_node_t> root_ui;
    model_t();
    std::shared_ptr<model_node_t> getRoot();
    const std::vector<std::shared_ptr<model_node_t>>& getShapes() const;
    void addShape(std::unique_ptr<shape_t> shape);
    void addShapeToParent(int parent_ui_id, std::unique_ptr<shape_t> shape);
    void removeLastShape();
    std::shared_ptr<model_node_t> getCurrentShape();
    std::shared_ptr<model_node_t> getLastNode();
    void rotateModel(char axis, bool positive);
    void render();
    size_t getShapeCount() const;
    void clear();
    void save(const std::string& filename);
    bool load(const std::string& filename);
};


// The core hierarchical node for rendering
class HNode {
public:
    std::unique_ptr<shape_t> shape;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 scale;
    std::vector<std::unique_ptr<HNode>> children;
    HNode* parent;

    HNode(std::unique_ptr<shape_t> s, HNode* p = nullptr);
    void render(const glm::mat4& parentTransform = glm::mat4(1.0f));
    void addChild(std::unique_ptr<HNode> child);
    glm::mat4 getLocalTransform() const;
    glm::mat4 getGlobalTransform() const;
};

#endif