#include "HIERARCHIAL.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

// --- model_node_t Method Definitions ---

model_node_t::model_node_t(std::shared_ptr<shape_t> s, ShapeType t)
    : id(next_id++), shape(std::move(s)), type(t) {}

void model_node_t::addChild(const std::shared_ptr<model_node_t>& child) {
    child->parent = shared_from_this();
    children.push_back(child);
}

glm::mat4 model_node_t::getTransform() const {
    return translation * rotation * scale;
}

// --- HNode Method Definitions ---

HNode::HNode(std::unique_ptr<shape_t> s, HNode* p)
    : shape(std::move(s)), parent(p) {
    rotation = glm::mat4(1.0f);
    translation = glm::mat4(1.0f);
    scale = glm::mat4(1.0f);
}

void HNode::render(const glm::mat4& parentTransform) {
    glm::mat4 modelMatrix = parentTransform * translation * rotation * scale;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }

    if (shape) {
        shape->draw();
    }

    for (auto& child : children) {
        child->render(modelMatrix);
    }
}

void HNode::addChild(std::unique_ptr<HNode> child) {
    child->parent = this;
    children.push_back(std::move(child));
}

glm::mat4 HNode::getLocalTransform() const {
    return translation * rotation * scale;
}

glm::mat4 HNode::getGlobalTransform() const {
    if (parent) {
        return parent->getGlobalTransform() * getLocalTransform();
    }
    return getLocalTransform();
}

// --- model_t Method Definitions ---

model_t::model_t() {
    std::unique_ptr<shape_t> emptyShape(nullptr);
    root_hnode = std::make_unique<HNode>(std::move(emptyShape));

    root_ui = std::make_shared<model_node_t>();
    root_ui->id = next_id++;
    root_ui->type = SPHERE_SHAPE; // placeholder
    root_ui->hnode_ptr = root_hnode.get();
    shapes.push_back(root_ui);
}

std::shared_ptr<model_node_t> model_t::findMNodeById(int id) {
    for (auto &m : shapes) if (m->id == id) return m;
    return nullptr;
}

bool model_t::removeHChildFromParent(HNode* parent, HNode* target) {
    if (!parent) return false;
    auto &vec = parent->children;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if (it->get() == target) {
            vec.erase(it);
            return true;
        }
    }
    return false;
}

std::shared_ptr<model_node_t> model_t::getRoot() {
    return shapes.empty() ? nullptr : shapes.front();
}

const std::vector<std::shared_ptr<model_node_t>>& model_t::getShapes() const { return shapes; }

void model_t::addShape(std::unique_ptr<shape_t> shape) {
    addShapeToParent(getRoot()->id, std::move(shape));
}

void model_t::addShapeToParent(int parent_ui_id, std::unique_ptr<shape_t> shape) {
    std::shared_ptr<model_node_t> parent_ui = findMNodeById(parent_ui_id);
    if (!parent_ui) {
        parent_ui = getRoot();
        if (!parent_ui) return;
    }

    std::unique_ptr<HNode> temp_hnode = std::make_unique<HNode>(std::move(shape));
    HNode* raw_ptr = temp_hnode.get();

    HNode* parent_hnode = parent_ui->hnode_ptr;
    if (!parent_hnode) {
        parent_hnode = root_hnode.get();
    }
    parent_hnode->addChild(std::move(temp_hnode));

    auto mnode = std::make_shared<model_node_t>();
    mnode->id = next_id++;
    mnode->type = raw_ptr->shape ? raw_ptr->shape->shapetype : SPHERE_SHAPE;
    mnode->translation = raw_ptr->translation;
    mnode->rotation = raw_ptr->rotation;
    mnode->scale = raw_ptr->scale;
    if (raw_ptr->shape && !raw_ptr->shape->colors.empty()) {
        mnode->color = raw_ptr->shape->colors[0];
    }
    mnode->hnode_ptr = raw_ptr;
    parent_ui->addChild(mnode);
    shapes.push_back(mnode);
}

void model_t::removeLastShape() {
    if (shapes.size() <= 1) return;
    auto last = shapes.back();
    HNode* target_h = last->hnode_ptr;
    auto parent_ui = last->parent.lock();
    HNode* parent_h = parent_ui ? parent_ui->hnode_ptr : root_hnode.get();

    if (parent_ui) {
        for (auto it = parent_ui->children.begin(); it != parent_ui->children.end(); ++it) {
            if ((*it)->id == last->id) {
                parent_ui->children.erase(it);
                break;
            }
        }
    }

    if (parent_h && target_h) {
        removeHChildFromParent(parent_h, target_h);
    }

    shapes.pop_back();
}

std::shared_ptr<model_node_t> model_t::getCurrentShape() {
    if (shapes.size() <= 1) return nullptr;
    return shapes.back();
}

std::shared_ptr<model_node_t> model_t::getLastNode() {
    if (shapes.size() <= 1) return nullptr;
    return shapes.back();
}

void model_t::rotateModel(char axis, bool positive) {
    float ang = glm::radians(5.0f) * (positive ? 1.0f : -1.0f);
    if (axis == 'X') root_hnode->rotation = glm::rotate(root_hnode->rotation, ang, glm::vec3(1,0,0));
    else if (axis == 'Y') root_hnode->rotation = glm::rotate(root_hnode->rotation, ang, glm::vec3(0,1,0));
    else if (axis == 'Z') root_hnode->rotation = glm::rotate(root_hnode->rotation, ang, glm::vec3(0,0,1));
}

void model_t::render() {
    if (root_hnode) root_hnode->render();
}

size_t model_t::getShapeCount() const {
    return (shapes.size() <= 1) ? 0 : shapes.size() - 1;
}

void model_t::clear() {
    std::unique_ptr<shape_t> emptyShape(nullptr);
    root_hnode = std::make_unique<HNode>(std::move(emptyShape));
    shapes.clear();
    auto new_root_ui = std::make_shared<model_node_t>();
    new_root_ui->id = next_id++;
    shapes.push_back(new_root_ui);
    root_ui = new_root_ui;
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
        if (s) s->setColor(e.color);
        addShapeToParent(e.parent_id, std::move(s));
        auto new_node = getLastNode();
        new_node->id = e.id;
        new_node->hnode_ptr->translation = e.translation;
        new_node->hnode_ptr->rotation = e.rotation;
        new_node->hnode_ptr->scale = e.scale;
        new_node->translation = e.translation;
        new_node->rotation = e.rotation;
        new_node->scale = e.scale;
        id_to_node[new_node->id] = new_node;
    }
    file.close();
    std::cout << "Model loaded from " << filename << std::endl;
    return true;
}