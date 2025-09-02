#ifndef SHAPE_H
#define SHAPE_H
#include <iostream>
#include <vector>
#include <memory>
#include <GL/glew.h>   
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Shape Types
enum ShapeType {
    SPHERE_SHAPE,
    CONE_SHAPE,
    BOX_SHAPE,
    CYLINDER_SHAPE
};

// Base Class
class shape_t {
public:
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> colors;
    std::vector<unsigned int> indices;

    GLuint VAO = 0, VBO = 0, CBO = 0, EBO = 0;
    ShapeType shapetype;
    unsigned int level;
    shape_t() : level(1) {}  
   shape_t(unsigned int tesselation_level) : level(tesselation_level) {
        if (level < 1) level = 1;
        if (level > 4) level = 4;
    }

    virtual ~shape_t() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (CBO) glDeleteBuffers(1, &CBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    ShapeType getType() const { return shapetype; }

    virtual void generateGeometry() = 0;
    unsigned int getLevel() const { return level; }
    void setLevel(unsigned int l) {
        if (l < 1) l = 1;
        if (l > 4) l = 4;
        if (level != l) {
            level = l;
            generateGeometry();
            VAO = VBO = CBO = EBO = 0; // force GPU buffer update
        }}
    virtual void setColor(const glm::vec4& c) {
        // Ensure color array matches vertices
        if (vertices.empty()) {
            colors.assign(1, c);
        } else {
            colors.assign(vertices.size(), c);
        }

        // Update GPU buffer if already created
        if (CBO != 0) {
            glBindBuffer(GL_ARRAY_BUFFER, CBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    void setupBuffers() {
        if (VAO != 0) return; // Already initialized

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Vertex positions
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(glm::vec4),
                     vertices.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        // Ensure colors exist
        if (colors.empty()) {
            colors.assign(vertices.size(), glm::vec4(1.0f)); // default white
        }

        // Vertex colors
        glGenBuffers(1, &CBO);
        glBindBuffer(GL_ARRAY_BUFFER, CBO);
        glBufferData(GL_ARRAY_BUFFER,
                     colors.size() * sizeof(glm::vec4),
                     colors.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);

        // Indices
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(),
                     GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    virtual void draw(const glm::mat4& MVP, GLuint shaderProgram) {
        if (VAO == 0) {
            generateGeometry();
            setupBuffers();
        }

        // Upload MVP
        GLint mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
        if (mvpLoc != -1) {
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void changeTesselation(int delta) {
        int newLevel = static_cast<int>(level) + delta;
        if (newLevel < 1) newLevel = 1;
        if (newLevel > 6) newLevel = 6;  // Match constructor limit
        setLevel(static_cast<unsigned int>(newLevel));
    }
};
// SphereS
class sphere_t : public shape_t {
public:
    sphere_t(unsigned int tesselation_level = 1) : shape_t(tesselation_level) {
        shapetype = SPHERE_SHAPE;
    }

    void generateGeometry() override {
        vertices.clear();
        colors.clear();
        indices.clear();

        unsigned int stacks = 10 * level;
        unsigned int slices = 10 * level;

        for (unsigned int i = 0; i <= stacks; ++i) {
            float phi = glm::pi<float>() * i / stacks;
            for (unsigned int j = 0; j <= slices; ++j) {
                float theta = 2.0f * glm::pi<float>() * j / slices;
                float x = sin(phi) * cos(theta);
                float y = cos(phi);
                float z = sin(phi) * sin(theta);

                vertices.emplace_back(x, y, z, 1.0f);
                colors.emplace_back((x+1)/2, (y+1)/2, (z+1)/2, 1.0f);
            }
        }

        for (unsigned int i = 0; i < stacks; ++i) {
            for (unsigned int j = 0; j < slices; ++j) {
                unsigned int first = i * (slices + 1) + j;
                unsigned int second = first + slices + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }
    }
};

// Cone
class cone_t : public shape_t {
public:
    cone_t(unsigned int tesselation_level = 2) : shape_t(tesselation_level) {
        shapetype = CONE_SHAPE;
    }

    void generateGeometry() override {
        vertices.clear();
        colors.clear();
        indices.clear();

        unsigned int slices = 20 * level;
        vertices.emplace_back(0, 1, 0, 1); // top
        colors.emplace_back(1, 0, 0, 1);
         // Center of base
        vertices.emplace_back(0, -1, 0, 1);
        colors.emplace_back(0, 0, 1, 1);  // blue

        for (unsigned int i = 0; i <= slices; ++i) {
            float theta = 2.0f * glm::pi<float>() * i / slices;
            float x = cos(theta);
            float z = sin(theta);
            vertices.emplace_back(x, -1, z, 1);
            colors.emplace_back(0, 1, 0, 1);
        }

        for (unsigned int i = 1; i <= slices; ++i) {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i+1);
        }for (unsigned int i = 0; i < slices; ++i) {
            unsigned int apex = 0;
            unsigned int v1 = 2 + i;
            unsigned int v2 = 2 + (i + 1);

            indices.push_back(apex);
            indices.push_back(v1);
            indices.push_back(v2);
        }

        // --- Base (fan) ---
        for (unsigned int i = 0; i < slices; ++i) {
            unsigned int center = 1;
            unsigned int v1 = 2 + i;
            unsigned int v2 = 2 + (i + 1);

            indices.push_back(center);
            indices.push_back(v2);
            indices.push_back(v1);
    }
}};

// Box
/*class box_t : public shape_t {
public:

    box_t(unsigned int tesselation_level = 1) : shape_t(tesselation_level) {
        shapetype = BOX_SHAPE;
    }

    void generateGeometry() override {
        vertices.clear();
        colors.clear();
        indices.clear();

        glm::vec4 v[] = {
            {-1,-1,-1,1}, {1,-1,-1,1}, {1,1,-1,1}, {-1,1,-1,1},
            {-1,-1, 1,1}, {1,-1, 1,1}, {1,1, 1,1}, {-1,1, 1,1}
        };

        for (int i = 0; i < 8; i++) {
            vertices.push_back(v[i]);
            colors.emplace_back((i&1)?1:0, (i&2)?1:0, (i&4)?1:0, 1);
        }

        unsigned int idx[] = {
            0,1,2, 2,3,0,  // back
            4,5,6, 6,7,4,  // front
            0,4,7, 7,3,0,  // left
            1,5,6, 6,2,1,  // right
            3,2,6, 6,7,3,  // top
            0,1,5, 5,4,0   // bottom
        };
        indices.assign(idx, idx+36);
    }
};
*/
class box_t : public shape_t {
public:
    box_t(unsigned int tesselation_level = 1) : shape_t(tesselation_level) {
        shapetype = BOX_SHAPE;
    }

    void generateGeometry() override {
        vertices.clear();
        colors.clear();
        indices.clear();

        unsigned int n = level; // tesselation subdivisions per edge
        if (n < 1) n = 1;

        // Each face spans from -1 to 1 in both axes
        auto addFace = [&](glm::vec3 origin, glm::vec3 uDir, glm::vec3 vDir) {
            unsigned int startIndex = vertices.size();

            for (unsigned int i = 0; i <= n; ++i) {
                for (unsigned int j = 0; j <= n; ++j) {
                    glm::vec3 pos = origin + uDir * (float(i)/n) * 2.0f + vDir * (float(j)/n) * 2.0f - (uDir+vDir); 
                    vertices.emplace_back(pos, 1.0f);
                    colors.emplace_back((pos.x+1)/2, (pos.y+1)/2, (pos.z+1)/2, 1.0f);
                }
            }

            // Add indices
            for (unsigned int i = 0; i < n; ++i) {
                for (unsigned int j = 0; j < n; ++j) {
                    unsigned int row1 = i * (n+1) + j + startIndex;
                    unsigned int row2 = (i+1) * (n+1) + j + startIndex;

                    // Triangle 1
                    indices.push_back(row1);
                    indices.push_back(row2);
                    indices.push_back(row1 + 1);

                    // Triangle 2
                    indices.push_back(row2);
                    indices.push_back(row2 + 1);
                    indices.push_back(row1 + 1);
                }
            }
        };

        // Define 6 faces with origin, uDir, vDir
        addFace(glm::vec3(-1,-1,-1), glm::vec3(2,0,0), glm::vec3(0,2,0)); // back
        addFace(glm::vec3(-1,-1, 1), glm::vec3(2,0,0), glm::vec3(0,2,0)); // front
        addFace(glm::vec3(-1,-1,-1), glm::vec3(0,0,2), glm::vec3(0,2,0)); // left
        addFace(glm::vec3( 1,-1,-1), glm::vec3(0,0,2), glm::vec3(0,2,0)); // right
        addFace(glm::vec3(-1, 1,-1), glm::vec3(2,0,0), glm::vec3(0,0,2)); // top
        addFace(glm::vec3(-1,-1,-1), glm::vec3(2,0,0), glm::vec3(0,0,2)); // bottom
    }
};

// Cylinder
class cylinder_t : public shape_t {
public:
    cylinder_t(unsigned int tesselation_level = 2) : shape_t(tesselation_level) {
        shapetype = CYLINDER_SHAPE;
    }

    void generateGeometry() override {
        std::cout << "=== Cylinder generateGeometry() called ===" << std::endl;
        vertices.clear();
        colors.clear();
        indices.clear();

        unsigned int slices = 20 * level;
        std::cout << "Tesselation level: " << level << std::endl;
        std::cout << "Slices: " << slices << std::endl;
        
        // Generate vertices (this part is correct)
        for (unsigned int i = 0; i <= slices; ++i) {
            float theta = 2.0f * glm::pi<float>() * i / slices;
            float x = cos(theta);
            float z = sin(theta);
            
            vertices.emplace_back(x, 1, z, 1);   // Top vertex (even index)
            colors.emplace_back(1, 0, 0, 1);     // Red
            
            vertices.emplace_back(x, -1, z, 1);  // Bottom vertex (odd index)  
            colors.emplace_back(0, 0, 1, 1);     // Blue
            
            if (i % 10 == 0) { // Debug every 10th iteration
                std::cout << "Iteration " << i << ": Added vertices at (" << x << ", 1, " << z << ") and (" << x << ", -1, " << z << ")" << std::endl;
            }
        }
        
        std::cout << "After vertex generation: " << vertices.size() << " vertices" << std::endl;
        
        // Fixed index generation
        for (unsigned int i = 0; i < slices; ++i) {
            unsigned int curr = i * 2;      // Curt pair start
            unsigned int next = ((i + 1)%slices)* 2; // Next pair start
            
            // Triangle 1: curr_top, curr_bottom, next_top
            indices.push_back(curr);        // Current top
            indices.push_back(curr + 1);    // Current bottom
            indices.push_back(next);        // Next top
            
            // Triangle 2: curr_bottom, next_bottom, next_top  
            indices.push_back(curr + 1);    // Current bottom
            indices.push_back(next + 1);    // Next bottom
            indices.push_back(next);        // Next top
            
            if (i % 10 == 0) { // Debug every 10th iteration
                std::cout << "Triangle pair " << i << ": indices (" << curr << "," << curr+1 << "," << next << ") and (" << curr+1 << "," << next+1 << "," << next << ")" << std::endl;
            }
        }
        
        std::cout << "After index generation: " << indices.size() << " indices" << std::endl;
        std::cout << "=== End generateGeometry() ===" << std::endl;
    }
};
#endif // SHAPE_H
