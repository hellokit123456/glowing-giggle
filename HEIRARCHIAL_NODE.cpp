#include "HIERARCHIAL.h"
#include "shape.h" // Include shape header for derived types in load()
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>


model_node_t::model_node_t(std::shared_ptr<shape_t> s, ShapeType t)
    : id(next_id++), shape(std::move(s)), type(t) {}

void model_node_t::addChild(const std::shared_ptr<model_node_t>& child) {
    child->parent = shared_from_this();
    children.push_back(child);
}

glm::mat4 model_node_t::getTransform() const {
    return translation * rotation * scale;
}

// model_t Method Definitions

model_t::model_t() {
    // Create a single root node for the scene
    root_node = std::make_shared<model_node_t>(nullptr, SPHERE_SHAPE); // Placeholder type
    root_node->id = next_id++;
    shapes.push_back(root_node);
}

std::shared_ptr<model_node_t> model_t::findMNodeById(int id) {
    for (auto &m : shapes) if (m->id == id) return m;
    return nullptr;
}

std::shared_ptr<model_node_t> model_t::getRoot() {
    return root_node;
}

const std::vector<std::shared_ptr<model_node_t>>& model_t::getShapes() const { return shapes; }

void model_t::addShape(std::unique_ptr<shape_t> shape) {
    addShapeToParent(getRoot()->id, std::move(shape));
}

void model_t::addShapeToParent(int parent_ui_id, std::unique_ptr<shape_t> shape) {
    std::shared_ptr<model_node_t> parent_node = findMNodeById(parent_ui_id);
    if (!parent_node) {
        parent_node = getRoot();
        if (!parent_node) return;
    }

    // Convert unique_ptr to shared_ptr and create the node
    std::shared_ptr<shape_t> shared_shape(std::move(shape));
    auto new_node = std::make_shared<model_node_t>(shared_shape, shared_shape ? shared_shape->shapetype : SPHERE_SHAPE);
    
    if (shared_shape && !shared_shape->colors.empty()) {
        new_node->color = shared_shape->colors[0];
    }
    
    parent_node->addChild(new_node);
    shapes.push_back(new_node);
}

void model_t::removeLastShape() {
    if (shapes.size() <= 1) return; // Can't remove the root
    
    auto last_node = shapes.back();
    shapes.pop_back();

    if (auto parent_node = last_node->parent.lock()) {
        auto& children = parent_node->children;
        for (auto it = children.begin(); it != children.end(); ++it) {
            if ((*it)->id == last_node->id) {
                children.erase(it);
                break;
            }
        }
    }
}

std::shared_ptr<model_node_t> model_t::getCurrentShape() {
    if (shapes.size() <= 1) return getRoot();
    return shapes.back();
}

std::shared_ptr<model_node_t> model_t::getLastNode() {
    if (shapes.empty()) return nullptr;
    return shapes.back();
}

void model_t::rotateModel(char axis, bool positive) {
    float ang = glm::radians(5.0f) * (positive ? 1.0f : -1.0f);
    if (axis == 'X') root_node->rotation = glm::rotate(root_node->rotation, ang, glm::vec3(1,0,0));
    else if (axis == 'Y') root_node->rotation = glm::rotate(root_node->rotation, ang, glm::vec3(0,1,0));
    else if (axis == 'Z') root_node->rotation = glm::rotate(root_node->rotation, ang, glm::vec3(0,0,1));
}

size_t model_t::getShapeCount() const {
    return (shapes.size() <= 1) ? 0 : shapes.size() - 1;
}

void model_t::clear() {
    shapes.clear();
    root_node = std::make_shared<model_node_t>(nullptr, SPHERE_SHAPE);
    root_node->id = next_id++;
    shapes.push_back(root_node);
}

void model_t::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to save model to " << filename << std::endl;
        return;
    }
    file << "MODEL_FILE_VERSION 1.0\n";
    file << "SHAPE_COUNT " << getShapeCount() << "\n";
    for (size_t i = 1; i < shapes.size(); ++i) {
        const auto& m = shapes[i];
        file << "SHAPE " << m->id << "\n";
        file << "TYPE " << static_cast<int>(m->type) << "\n";
        file << "TRANSLATION ";
        const float* tptr = glm::value_ptr(m->translation);
        for (int k = 0; k < 16; ++k) file << tptr[k] << " ";
        file << "\n";
        file << "ROTATION ";
        const float* rptr = glm::value_ptr(m->rotation);
        for (int k = 0; k < 16; ++k) file << rptr[k] << " ";
        file << "\n";
        file << "SCALE ";
        const float* sptr = glm::value_ptr(m->scale);
        for (int k = 0; k < 16; ++k) file << sptr[k] << " ";
        file << "\n";
        int parent_id = -1;
        if (auto p = m->parent.lock()) parent_id = p->id;
        file << "PARENT " << parent_id << "\n";
        file << "COLOR " << m->color.r << " " << m->color.g << " " << m->color.b << " " << m->color.a << "\n";
    }
    file.close();
    std::cout << "Model saved to " << filename << std::endl;
}

bool model_t::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to load model from " << filename << std::endl;
        return false;
    }

    struct Entry {
        int id; ShapeType type; glm::mat4 translation, rotation, scale; int parent_id; glm::vec4 color;
    };
    std::vector<Entry> entries;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "SHAPE") {
            Entry e{};
            iss >> e.id;
            std::streampos lastPos;
            while (true) {
                lastPos = file.tellg();
                if (!std::getline(file, line)) break;
                std::istringstream ps(line);
                std::string prop; ps >> prop;
                if (prop == "TYPE") { int t; ps >> t; e.type = static_cast<ShapeType>(t); }
                else if (prop == "TRANSLATION") { float* ptr = glm::value_ptr(e.translation); for (int k=0; k<16; ++k) ps >> ptr[k]; }
                else if (prop == "ROTATION") { float* ptr = glm::value_ptr(e.rotation); for (int k=0; k<16; ++k) ps >> ptr[k]; }
                else if (prop == "SCALE") { float* ptr = glm::value_ptr(e.scale); for (int k=0; k<16; ++k) ps >> ptr[k]; }
                else if (prop == "PARENT") { ps >> e.parent_id; }
                else if (prop == "COLOR") { ps >> e.color.r >> e.color.g >> e.color.b >> e.color.a; }
                else { file.seekg(lastPos); break; }
            }
            entries.push_back(e);
        }
    }

    clear();
    std::unordered_map<int, std::shared_ptr<model_node_t>> id_to_node;
    id_to_node[getRoot()->id] = getRoot();

    for (const auto& e : entries) {
        std::unique_ptr<shape_t> s;
        switch (e.type) {
            case SPHERE_SHAPE: s = std::make_unique<sphere_t>(2); break;
            case CYLINDER_SHAPE: s = std::make_unique<cylinder_t>(2); break;
            case BOX_SHAPE: s = std::make_unique<box_t>(2); break;
            case CONE_SHAPE: s = std::make_unique<cone_t>(2); break;
        }
        
        addShapeToParent(e.parent_id, std::move(s));
        
        auto new_node = getLastNode();
        if (new_node->shape) {
            new_node->shape->setColor(e.color);
        }
        new_node->id = e.id;
        new_node->translation = e.translation;
        new_node->rotation = e.rotation;
        new_node->scale = e.scale;
        id_to_node[new_node->id] = new_node;
    }
    file.close();
    std::cout << "Model loaded from " << filename << std::endl;
    return true;
}
