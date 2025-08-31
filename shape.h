#ifndef SHAPE_H
#define SHAPE_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum ShapeType {
    SPHERE_SHAPE,
    CYLINDER_SHAPE,
    BOX_SHAPE,
    CONE_SHAPE
};

class shape_t {
protected:
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> colors;
    ShapeType shapetype;
    unsigned int level;

public:
    // Pure virtual constructor - derived classes must implement
    shape_t(unsigned int tesselation_level) : level(tesselation_level) {
        if (level > 4) level = 4;
        if (level < 0) level = 0;
    }
    
    virtual ~shape_t() = default;
    
    // Pure virtual draw method
    virtual void draw() = 0;
    
    // Getters
    const std::vector<glm::vec4>& getVertices() const { return vertices; }
    const std::vector<glm::vec4>& getColors() const { return colors; }
    ShapeType getShapeType() const { return shapetype; }
    unsigned int getLevel() const { return level; }
    
    // Setters
    void setColor(const glm::vec4& color) {
        for (auto& c : colors) {
            c = color;
        }
    }
    
    // Transformation methods - to be called from input handler
    virtual void rotate(char axis, bool positive) = 0;
    virtual void translate(char axis, bool positive) = 0;
    virtual void scale(char axis, bool positive) = 0;
    
    // Get centroid of the shape
    glm::vec3 getCentroid() const {
        glm::vec3 centroid(0.0f);
        for (const auto& vertex : vertices) {
            centroid += glm::vec3(vertex);
        }
        if (!vertices.empty()) {
            centroid /= vertices.size();
        }
        return centroid;
    }
};

class sphere_t : public shape_t {
private:
    float radius;
    glm::mat4 transform;
    
    void generateSphere();
    
public:
    sphere_t(unsigned int tesselation_level, float r = 1.0f) 
        : shape_t(tesselation_level), radius(r), transform(1.0f) {
        shapetype = SPHERE_SHAPE;
        generateSphere();
    }
    
    void draw() override;
    void rotate(char axis, bool positive) override;
    void translate(char axis, bool positive) override;
    void scale(char axis, bool positive) override;
};

class cylinder_t : public shape_t {
private:
    float radius;
    float height;
    glm::mat4 transform;
    
    void generateCylinder();
    
public:
    cylinder_t(unsigned int tesselation_level, float r = 1.0f, float h = 2.0f)
        : shape_t(tesselation_level), radius(r), height(h), transform(1.0f) {
        shapetype = CYLINDER_SHAPE;
        generateCylinder();
    }
    
    void draw() override;
    void rotate(char axis, bool positive) override;
    void translate(char axis, bool positive) override;
    void scale(char axis, bool positive) override;
};

class box_t : public shape_t {
private:
    float width, height, depth;
    glm::mat4 transform;
    
    void generateBox();
    
public:
    box_t(unsigned int tesselation_level, float w = 1.0f, float h = 1.0f, float d = 1.0f)
        : shape_t(tesselation_level), width(w), height(h), depth(d), transform(1.0f) {
        shapetype = BOX_SHAPE;
        generateBox();
    }
    
    void draw() override;
    void rotate(char axis, bool positive) override;
    void translate(char axis, bool positive) override;
    void scale(char axis, bool positive) override;
};

class cone_t : public shape_t {
private:
    float radius;
    float height;
    glm::mat4 transform;
    
    void generateCone();
    
public:
    cone_t(unsigned int tesselation_level, float r = 1.0f, float h = 2.0f)
        : shape_t(tesselation_level), radius(r), height(h), transform(1.0f) {
        shapetype = CONE_SHAPE;
        generateCone();
    }
    
    void draw() override;
    void rotate(char axis, bool positive) override;
    void translate(char axis, bool positive) override;
    void scale(char axis, bool positive) override;
};

// Model Node for hierarchical structure
class model_node_t {
public:
    std::shared_ptr<shape_t> shape;
    glm::mat4 rotation;
    glm::mat4 scale;
    glm::mat4 translation;
    
    std::vector<std::shared_ptr<model_node_t>> children;
    std::shared_ptr<model_node_t> parent;
    
    model_node_t() {
        rotation = glm::mat4(1.0f);
        scale = glm::mat4(1.0f);
        translation = glm::mat4(1.0f);
        parent = nullptr;
    }
    
    glm::mat4 getTransform() const {
        return translation * rotation * scale;
    }
    
    glm::mat4 getGlobalTransform() const {
        if (parent) {
            return parent->getGlobalTransform() * getTransform();
        }
        return getTransform();
    }
    
    void addChild(std::shared_ptr<model_node_t> child) {
        child->parent = shared_from_this();
        children.push_back(child);
    }
    
    glm::vec3 getCentroid() const {
        if (shape) {
            glm::vec4 centroid = getGlobalTransform() * glm::vec4(shape->getCentroid(), 1.0f);
            return glm::vec3(centroid);
        }
        return glm::vec3(0.0f);
    }
};

class model_t {
private:
    std::shared_ptr<model_node_t> root;
    std::vector<std::shared_ptr<model_node_t>> shapes; // For easy access to all shapes
    
public:
    model_t() {
        root = std::make_shared<model_node_t>();
    }
    
    void addShape(std::shared_ptr<shape_t> shape);
    void removeLastShape();
    void clear();
    
    std::shared_ptr<model_node_t> getCurrentShape() {
        return shapes.empty() ? nullptr : shapes.back();
    }
    
    std::shared_ptr<model_node_t> getRoot() { return root; }
    const std::vector<std::shared_ptr<model_node_t>>& getShapes() const { return shapes; }
    
    void draw();
    
    // File operations
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);
    
    // Get model centroid
    glm::vec3 getModelCentroid() const;
    
    // Model transformations (for inspection mode)
    void rotateModel(char axis, bool positive);
};

#endif // SHAPE_H
