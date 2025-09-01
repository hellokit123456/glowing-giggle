#include <GL/glew.h>     // MUST come first
#include <GLFW/glfw3.h>  // then GLFW
#include <glm/glm.hpp>   // then GLM
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <string>
#include <sstream>
#include "shape.h"
// Global or passed-in camera matrices
glm::mat4 projection;
glm::mat4 view;
GLuint shaderProgram=0;
enum Mode { MODELLING, INSPECTION };
enum TransformMode { NONE, ROTATE, TRANSLATE, SCALE };

GLuint createShaderProgram() {
    const char* vertexShaderSrc = R"(
    #version 330 core
    layout(location = 0) in vec4 aPos;
    layout(location = 1) in vec4 aColor;
    uniform mat4 MVP;
    out vec4 fragColor;
    void main() {
        gl_Position = MVP * aPos;
        fragColor = aColor;
    })";

    const char* fragmentShaderSrc = R"(
    #version 330 core
    in vec4 fragColor;
    out vec4 color;
    void main() {
        color = fragColor;
    })";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Hierarchical Node class (based on Tutorial 7)
class HNode {
public:
    std::unique_ptr<shape_t> shape;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 scale;
    std::vector<std::unique_ptr<HNode>> children;

    HNode(std::unique_ptr<shape_t> s) : shape(std::move(s)) {
        rotation = glm::mat4(1.0f);
        translation = glm::mat4(1.0f);
        scale = glm::mat4(1.0f);
    }

    // parentTransform is the accumulated transform from ancestors
    void render(const glm::mat4& parentTransform = glm::mat4(1.0f)) {
        glm::mat4 modelMatrix = parentTransform * translation * rotation * scale;

        // Set the per-node "model" uniform (view & projection should be set once per-frame)
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

    void addChild(std::unique_ptr<HNode> child) {
        children.push_back(std::move(child));
    }
};


// Cylinder implementation
class cylinder_t : public shape_t {
public:
    cylinder_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = CYLINDER_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        int segments = 6 + level * 3; // 6, 9, 12, 15, 18 segments based on level
        
        // Side vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -0.5f, z, 1.0f); // bottom
            vertices.emplace_back(x, 0.5f, z, 1.0f);  // top
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        }
        
        // Side indices
        for (int i = 0; i < segments; ++i) {
            int base = i * 2;
            
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            
            indices.push_back(base + 1);
            indices.push_back(base + 3);
            indices.push_back(base + 2);
        }
        
        // Cap vertices and indices
        int centerBottom = vertices.size();
        vertices.emplace_back(0, -0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        int centerTop = vertices.size();
        vertices.emplace_back(0, 0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        for (int i = 0; i < segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -0.5f, z, 1.0f);
            vertices.emplace_back(x, 0.5f, z, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            
            int bottomVertex = centerBottom + 2 + i * 2;
            int topVertex = bottomVertex + 1;
            int nextBottomVertex = centerBottom + 2 + ((i + 1) % segments) * 2;
            int nextTopVertex = nextBottomVertex + 1;
            
            // Bottom cap
            indices.push_back(centerBottom);
            indices.push_back(nextBottomVertex);
            indices.push_back(bottomVertex);
            
            // Top cap
            indices.push_back(centerTop);
            indices.push_back(topVertex);
            indices.push_back(nextTopVertex);
        }
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Box implementation
class box_t : public shape_t {
public:
    box_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = BOX_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        // For a box, tessellation affects subdivision of faces
        int subdivisions = level + 1; // 1, 2, 3, 4, 5 subdivisions based on level
        
        // Generate vertices for each face with subdivisions
        generateFace(glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), subdivisions); // Front
        generateFace(glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), subdivisions); // Back
        generateFace(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), subdivisions); // Top
        generateFace(glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), subdivisions); // Bottom
        generateFace(glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), subdivisions); // Right
        generateFace(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), subdivisions); // Left
    }
    
    void generateFace(const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, int subdivisions) {
        int startVertex = vertices.size();
        glm::vec3 center = normal * 0.5f;
        
        // Generate grid of vertices for this face
        for (int i = 0; i <= subdivisions; ++i) {
            for (int j = 0; j <= subdivisions; ++j) {
                float u = (float)i / subdivisions - 0.5f;
                float v = (float)j / subdivisions - 0.5f;
                
                glm::vec3 pos = center + tangent * u + bitangent * v;
                vertices.emplace_back(pos.x, pos.y, pos.z, 1.0f);
                colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
            }
        }
        
        // Generate indices for this face
        for (int i = 0; i < subdivisions; ++i) {
            for (int j = 0; j < subdivisions; ++j) {
                int topLeft = startVertex + i * (subdivisions + 1) + j;
                int topRight = topLeft + 1;
                int bottomLeft = topLeft + (subdivisions + 1);
                int bottomRight = bottomLeft + 1;
                
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Cone implementation
class cone_t : public shape_t {
public:
    cone_t(unsigned int tesselation_level) : shape_t(tesselation_level) {
        shapetype = CONE_SHAPE;
        generateGeometry();
        setupBuffers();
    }
    
    void generateGeometry() {
        vertices.clear();
        colors.clear();
        indices.clear();
        
        int segments = 6 + level * 3; // 6, 9, 12, 15, 18 segments based on level
        
        // Apex vertex
        vertices.emplace_back(0, 0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        // Base center
        vertices.emplace_back(0, -0.5f, 0, 1.0f);
        colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        
        // Base vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = cos(angle);
            float z = sin(angle);
            
            vertices.emplace_back(x, -0.5f, z, 1.0f);
            colors.emplace_back(0.7f, 0.7f, 0.7f, 1.0f);
        }
        
        // Side triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(0); // apex
            indices.push_back(i + 2);
            indices.push_back(i + 3);
        }
        
        // Base triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(1); // center
            indices.push_back(i + 3);
            indices.push_back(i + 2);
        }
    }
    
    void draw() override {
        if (VAO == 0) setupBuffers();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};
// Model class using hierarchical nodes
// Mirror node used for UI / scene listing (non-owning pointer to HNode)
struct model_node_t {
    int id = -1;
    ShapeType type = SPHERE_SHAPE;
    glm::mat4 translation = glm::mat4(1.0f);
    glm::mat4 rotation = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    glm::vec4 color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);

    std::weak_ptr<model_node_t> parent;                      // for UI tree traversal
    std::vector<std::shared_ptr<model_node_t>> children;     // UI children
    HNode* hnode_ptr = nullptr;                              // non-owning pointer to the corresponding HNode
};

// Hierarchical model container (replacement)
class model_t {
private:
    std::unique_ptr<HNode> root_hnode;                       // owns full HNode tree
    std::vector<std::shared_ptr<model_node_t>> shapes;       // UI / scene-list mirrors (owned)
    int next_id = 0;

    // Helper: find model_node_t by id
    std::shared_ptr<model_node_t> findMNodeById(int id) {
        for (auto &m : shapes) if (m->id == id) return m;
        return nullptr;
    }

    // Helper: remove child HNode from a parent's children by raw pointer
    bool removeHChildFromParent(HNode* parent, HNode* target) {
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

public:
    model_t() {
        // create root HNode with no shape (null unique_ptr)
        std::unique_ptr<shape_t> emptyShape(nullptr);
        root_hnode = std::make_unique<HNode>(std::move(emptyShape));
        // create root UI node
        auto root_ui = std::make_shared<model_node_t>();
        root_ui->id = next_id++;
        root_ui->type = SPHERE_SHAPE; // meaningless for root, but set a default
        shapes.push_back(root_ui);
    }

    // Basic accessor for root UI node (scene tree root)
    std::shared_ptr<model_node_t> getRoot() {
        // root is the first element in shapes by construction
        return shapes.empty() ? nullptr : shapes.front();
    }

    const std::vector<std::shared_ptr<model_node_t>>& getShapes() const { return shapes; }

    // Add shape as a child of the root UI/HNode by default
    void addShape(std::unique_ptr<shape_t> shape) {
        addShapeToParent(getRoot()->id, std::move(shape));
    }

    // Add shape under parent UI node id (attach to that HNode)
    void addShapeToParent(int parent_ui_id, std::unique_ptr<shape_t> shape) {
        std::shared_ptr<model_node_t> parent_ui = findMNodeById(parent_ui_id);
        if (!parent_ui) {
            // fallback to root
            parent_ui = getRoot();
            if (!parent_ui) return;
        }

        // Create the new HNode (temporary holder)
        std::unique_ptr<HNode> temp_hnode = std::make_unique<HNode>(std::move(shape));
        HNode* raw_ptr = temp_hnode.get();

        // Attach to the parent's HNode children (move ownership)
        HNode* parent_hnode = parent_ui->hnode_ptr;
        if (!parent_hnode) {
            // if parent_hnode null (root UI), use root_hnode
            parent_hnode = root_hnode.get();
        }
        parent_hnode->children.push_back(std::move(temp_hnode));
        // note: raw_ptr is now owned by parent_hnode->children.back()

        // Create UI mirror node
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
        mnode->parent = parent_ui;
        parent_ui->children.push_back(mnode);
        shapes.push_back(mnode);
    }

    // Remove last added shape (both UI mirror and HNode in the tree)
    void removeLastShape() {
        if (shapes.size() <= 1) return; // keep root intact (root is shapes[0])
        auto last = shapes.back();
        HNode* target_h = last->hnode_ptr;
        // find its parent UI and parent HNode
        auto parent_ui = last->parent.lock();
        HNode* parent_h = parent_ui ? parent_ui->hnode_ptr : root_hnode.get();

        // remove UI entry from parent's children
        if (parent_ui) {
            for (auto it = parent_ui->children.begin(); it != parent_ui->children.end(); ++it) {
                if ((*it)->id == last->id) {
                    parent_ui->children.erase(it);
                    break;
                }
            }
        }

        // remove HNode child from parent_hnode->children
        if (parent_h && target_h) {
            removeHChildFromParent(parent_h, target_h);
        }

        // remove from shapes vector
        shapes.pop_back();
    }

    std::shared_ptr<model_node_t> getCurrentShape() {
        if (shapes.size() <= 1) return nullptr; // no actual shapes
        return shapes.back();
    }

    HNode* getLastNode() {
        auto cur = getCurrentShape();
        return cur ? cur->hnode_ptr : nullptr;
    }

    // rotate whole model (for inspection)
    void rotateModel(char axis, bool positive) {
        float ang = glm::radians(5.0f) * (positive ? 1.0f : -1.0f);
        // apply to root_hnode's transform (we will use root_hnode as global)
        if (axis == 'X') root_hnode->rotation = glm::rotate(root_hnode->rotation, ang, glm::vec3(1,0,0));
        else if (axis == 'Y') root_hnode->rotation = glm::rotate(root_hnode->rotation, ang, glm::vec3(0,1,0));
        else if (axis == 'Z') root_hnode->rotation = glm::rotate(root_hnode->rotation, ang, glm::vec3(0,0,1));
    }

    // Render: call root_hnode->render so hierarchical transforms propagate
    void render() {
        if (root_hnode) root_hnode->render();
    }

    size_t getShapeCount() const {
        // exclude root
        return (shapes.size() <= 1) ? 0 : shapes.size() - 1;
    }

    void clear() {
        // recreate empty root
        std::unique_ptr<shape_t> emptyShape(nullptr);
        root_hnode = std::make_unique<HNode>(std::move(emptyShape));
        shapes.clear();
        auto root_ui = std::make_shared<model_node_t>();
        root_ui->id = next_id++;
        shapes.push_back(root_ui);
    }

    // Save: traverse hierarchy and save parent-child relationships (PARENT <id>)
    void save(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to save model to " << filename << std::endl;
            return;
        }

        file << "MODEL_FILE_VERSION 1.0\n";
        file << "SHAPE_COUNT " << getShapeCount() << "\n";

        // For saving we traverse shapes vector (skipping the first root entry)
        for (size_t i = 1; i < shapes.size(); ++i) {
            const auto& m = shapes[i];
            file << "SHAPE " << m->id << "\n";
            file << "TYPE " << static_cast<int>(m->type) << "\n";

            // Save transform matrices
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

            // Parent id
            int parent_id = -1;
            if (auto p = m->parent.lock()) parent_id = p->id;
            file << "PARENT " << parent_id << "\n";

            // color
            file << "COLOR " << m->color.r << " " << m->color.g << " " << m->color.b << " " << m->color.a << "\n";
        }

        file.close();
        std::cout << "Model saved to " << filename << std::endl;
    }

    // Load: reconstruct shapes and hierarchy
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to load model from " << filename << std::endl;
            return false;
        }

        // temporary storage for entries
        struct Entry {
            int id;
            ShapeType type;
            glm::mat4 translation;
            glm::mat4 rotation;
            glm::mat4 scale;
            int parent_id;
            glm::vec4 color;
        };
        std::vector<Entry> entries;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            if (token == "SHAPE") {
                Entry e;
                iss >> e.id;
                // default
                e.type = SPHERE_SHAPE;
                e.translation = glm::mat4(1.0f);
                e.rotation = glm::mat4(1.0f);
                e.scale = glm::mat4(1.0f);
                e.parent_id = -1;
                e.color = glm::vec4(0.7f,0.7f,0.7f,1.0f);

                // read subsequent lines for this shape
                std::streampos lastPos;
                while (true) {
                    lastPos = file.tellg();
                    if (!std::getline(file, line)) break;
                    if (line.empty()) break;
                    std::istringstream ps(line);
                    std::string prop;
                    ps >> prop;
                    if (prop == "TYPE") {
                        int t; ps >> t; e.type = static_cast<ShapeType>(t);
                    } else if (prop == "TRANSLATION") {
                        float* ptr = glm::value_ptr(e.translation);
                        for (int k = 0; k < 16; ++k) ps >> ptr[k];
                    } else if (prop == "ROTATION") {
                        float* ptr = glm::value_ptr(e.rotation);
                        for (int k = 0; k < 16; ++k) ps >> ptr[k];
                    } else if (prop == "SCALE") {
                        float* ptr = glm::value_ptr(e.scale);
                        for (int k = 0; k < 16; ++k) ps >> ptr[k];
                    } else if (prop == "PARENT") {
                        ps >> e.parent_id;
                    } else if (prop == "COLOR") {
                        ps >> e.color.r >> e.color.g >> e.color.b >> e.color.a;
                    } else {
                        file.seekg(lastPos);
                        break;
                    }
                }
                entries.push_back(e);
            }
        }

        // Clear current model and recreate
        clear();

        // First create all nodes (under root by default), and map id->mnode
        std::unordered_map<int, std::shared_ptr<model_node_t>> id2mnode;
        for (auto &e : entries) {
            // create shape
            std::unique_ptr<shape_t> s;
            switch (e.type) {
                case SPHERE_SHAPE: s = std::make_unique<sphere_t>(2); break;
                case CYLINDER_SHAPE: s = std::make_unique<cylinder_t>(2); break;
                case BOX_SHAPE: s = std::make_unique<box_t>(2); break;
                case CONE_SHAPE: s = std::make_unique<cone_t>(2); break;
            }
            // set color
            s->setColor(e.color);

            // attach under root temporarily (we will reparent later)
            std::unique_ptr<HNode> temp = std::make_unique<HNode>(std::move(s));
            // set transforms
            temp->translation = e.translation;
            temp->rotation = e.rotation;
            temp->scale = e.scale;

            HNode* raw_ptr = temp.get();
            root_hnode->children.push_back(std::move(temp)); // temporary parent is root

            // create UI mirror
            auto mnode = std::make_shared<model_node_t>();
            mnode->id = e.id;
            mnode->type = e.type;
            mnode->translation = e.translation;
            mnode->rotation = e.rotation;
            mnode->scale = e.scale;
            mnode->color = e.color;
            mnode->hnode_ptr = raw_ptr;
            // add under root UI for now
            shapes.push_back(mnode);
            root_hnode->children.back()->shape; // no-op to silence unused
            id2mnode[e.id] = mnode;
        }

        // Now fix parents according to entries (reparent both UI and HNode)
        for (auto &e : entries) {
            auto mnode = id2mnode[e.id];
            int pid = e.parent_id;
            std::shared_ptr<model_node_t> parent_ui = (pid == -1) ? getRoot() : id2mnode[pid];
            // find HNode pointers
            HNode* child_h = mnode->hnode_ptr;
            HNode* new_parent_h = parent_ui->hnode_ptr ? parent_ui->hnode_ptr : root_hnode.get();

            // remove child_h from current parent (root) and move to new_parent_h
            // find current parent (search root_hnode tree for the parent owning child_h)
            // simple approach: remove from root_hnode children vector (since we temporarily put all under root)
            // Try to remove from root first
            removeHChildFromParent(root_hnode.get(), child_h);
            // then attach to new_parent_h
            new_parent_h->children.push_back(std::unique_ptr<HNode>(child_h)); 
            // WARNING: ownership transfer above is not valid because child_h is already owned by root_hnode->children memory we erased.
            // To avoid complex ownership juggling here, instead we'll rebuild tree cleanly below.
        }

        // The ownership juggling above is complex; simpler approach: reconstruct tree from entries cleanly:
        // Clear root and UI and then rebuild properly
        root_hnode = std::make_unique<HNode>(std::unique_ptr<shape_t>(nullptr));
        // Rebuild mnode map and hnode ownership mapping
        id2mnode.clear();
        shapes.clear();

        // First create all mnodes and raw HNode pointers stored temporarily
        struct TempNode { Entry e; std::unique_ptr<HNode> owned; HNode* raw; std::shared_ptr<model_node_t> mnode; };
        std::vector<TempNode> tempNodes;
        for (auto &e : entries) {
            std::unique_ptr<shape_t> s;
            switch (e.type) {
                case SPHERE_SHAPE: s = std::make_unique<sphere_t>(2); break;
                case CYLINDER_SHAPE: s = std::make_unique<cylinder_t>(2); break;
                case BOX_SHAPE: s = std::make_unique<box_t>(2); break;
                case CONE_SHAPE: s = std::make_unique<cone_t>(2); break;
            }
            s->setColor(e.color);
            TempNode tn;
            tn.e = e;
            tn.owned = std::make_unique<HNode>(std::move(s));
            tn.owned->translation = e.translation;
            tn.owned->rotation = e.rotation;
            tn.owned->scale = e.scale;
            tn.raw = tn.owned.get();
            tn.mnode = std::make_shared<model_node_t>();
            tn.mnode->id = e.id;
            tn.mnode->type = e.type;
            tn.mnode->translation = e.translation;
            tn.mnode->rotation = e.rotation;
            tn.mnode->scale = e.scale;
            tn.mnode->color = e.color;
            tn.mnode->hnode_ptr = tn.raw;
            tempNodes.push_back(std::move(tn));
        }

        // helper to find TempNode by id
        auto findTempById = [&](int id)->TempNode* {
            for (auto &t : tempNodes) if (t.e.id == id) return &t;
            return nullptr;
        };

        // attach each TempNode under the specified parent (or root)
        for (auto &t : tempNodes) {
            int pid = t.e.parent_id;
            if (pid == -1) {
                // attach to root_hnode
                root_hnode->children.push_back(std::move(t.owned));
                // root_hnode->children.back() now owns it; update mnode->hnode_ptr already set
                shapes.push_back(t.mnode);
                // parent of UI is root
                t.mnode->parent = getRoot();
                getRoot()->children.push_back(t.mnode);
            } else {
                // find parent's raw HNode pointer
                TempNode* parentTemp = findTempById(pid);
                if (parentTemp) {
                    parentTemp->owned->children.push_back(std::move(t.owned));
                    shapes.push_back(t.mnode);
                    // wire UI parent-child
                    t.mnode->parent = parentTemp->mnode;
                    parentTemp->mnode->children.push_back(t.mnode);
                } else {
                    // fallback: attach to root
                    root_hnode->children.push_back(std::move(t.owned));
                    shapes.push_back(t.mnode);
                    t.mnode->parent = getRoot();
                    getRoot()->children.push_back(t.mnode);
                }
            }
        }

        file.close();
        std::cout << "Model loaded from " << filename << std::endl;
        return true;
    }
};



// Global variables
Mode currentMode = MODELLING;
TransformMode transformMode = NONE;
char activeAxis = 'X';
std::unique_ptr<model_t> currentModel;
HNode* currentNode = nullptr;

// Camera variables
float cameraDistance = 5.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
glm::mat4 modelRotation = glm::mat4(1.0f);

// Function declarations
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleModellingKeys(int key);
void handleInspectionKeys(int key);
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
void renderScene();
void updateCamera();
void printInstructions();

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
        // Add shapes
        case GLFW_KEY_1:
            currentModel->addShape(std::make_unique<sphere_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Sphere added\n";
            break;
        case GLFW_KEY_2:
            currentModel->addShape(std::make_unique<cylinder_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cylinder added\n";
            break;
        case GLFW_KEY_3:
            currentModel->addShape(std::make_unique<box_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Box added\n";
            break;
        case GLFW_KEY_4:
            currentModel->addShape(std::make_unique<cone_t>(1));
            currentNode = currentModel->getLastNode();
            std::cout << "Cone added\n";
            break;
        case GLFW_KEY_5:
            currentModel->removeLastShape();
            currentNode = currentModel->getLastNode();
            std::cout << "Last shape removed\n";
            break;
        case GLFW_KEY_U: // Move UP to parent
            if (currentNode->parent) {
                currentNode = currentNode->parent;
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
void renderNode(model_node_t* node, const glm::mat4& parentTransform) {
    glm::mat4 modelMatrix = parentTransform * node->getTransform();

    if (node->shape) {
        glm::mat4 MVP = projection * view * modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"),
                           1, GL_FALSE, glm::value_ptr(MVP));
        node->shape->draw();
    }

    for (auto& child : node->children) {
        renderNode(child.get(), modelMatrix);
    }
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup camera
    projection = glm::perspective(glm::radians(45.0f),
                                  800.0f / 600.0f, 0.1f, 100.0f);
    view = glm::lookAt(glm::vec3(0, 0, 5),  // camera pos
                       glm::vec3(0, 0, 0),  // look at origin
                       glm::vec3(0, 1, 0)); // up

    if (currentModel && currentModel->root) {
        renderNode(currentModel->root.get(), glm::mat4(1.0f));
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Modeller", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
}



// Initialize GLEW
glewExperimental = GL_TRUE; // important for core profiles
if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW\n";
    return -1;
}

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Shaders
    shaderProgram = createShaderProgram();

    // Camera
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                       glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));

    // Scene + keyboard
    currentModel = std::make_unique<model_t>();
    glfwSetKeyCallback(window, keyCallback);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        renderScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

