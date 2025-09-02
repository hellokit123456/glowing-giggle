#ifndef SHAPE_H
#define SHAPE_H

#include <vector>
#include <memory>
#include <GL/glew.h>   
#include <glm/glm.hpp>

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
    void setColor(const glm::vec4& c) { 
        colors.assign(1, c); 
    }
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> colors;
    std::vector<unsigned int> indices;

    GLuint VAO = 0, VBO = 0, CBO = 0, EBO = 0;
    ShapeType shapetype;
    unsigned int level;

    // GPU setup
    void setupBuffers() {
        if (VAO != 0) return; // Already initialized

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Vertex positions
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec4),
                     vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        // Vertex colors
        glGenBuffers(1, &CBO);
        glBindBuffer(GL_ARRAY_BUFFER, CBO);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4),
                     colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);

        // Indices
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                     indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    shape_t(unsigned int tesselation_level = 1)
        : shapetype(SPHERE_SHAPE), level(tesselation_level) {
        if (level > 4) level = 4;
    }

    virtual ~shape_t() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (CBO) glDeleteBuffers(1, &CBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    ShapeType getType() const { return shapetype; }
    unsigned int getLevel() const { return level; }

    // Must be implemented by each shape
    virtual void generateGeometry() = 0;

    // Common draw method
    virtual void draw() {
        if (VAO == 0) {
            generateGeometry();
            setupBuffers();
        }
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Sphere
class sphere_t : public shape_t {
public:
    sphere_t(unsigned int tesselation_level = 2) : shape_t(tesselation_level) {
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
        }
    }
};

// Box
class box_t : public shape_t {
public:
virtual void setColor(const glm::vec4& color) {
    std::fill(colors.begin(), colors.end(), color);
    // Update GPU buffer if already initialized
    if (CBO != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, CBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());
    }
}
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

// Cylinder
class cylinder_t : public shape_t {
public:
    cylinder_t(unsigned int tesselation_level = 2) : shape_t(tesselation_level) {
        shapetype = CYLINDER_SHAPE;
    }

   void generateGeometry() override {
    vertices.clear();
    colors.clear();
    indices.clear();
    
    unsigned int slices = 20 * level;
    
    // Generate vertices (this part is correct)
    for (unsigned int i = 0; i <= slices; ++i) {
        float theta = 2.0f * glm::pi<float>() * i / slices;
        float x = cos(theta);
        float z = sin(theta);
        
        vertices.emplace_back(x, 1, z, 1);   // Top vertex (even index)
        colors.emplace_back(1, 0, 0, 1);     // Red
        
        vertices.emplace_back(x, -1, z, 1);  // Bottom vertex (odd index)  
        colors.emplace_back(0, 0, 1, 1);     // Blue
    }
    
    // Fixed index generation
    for (unsigned int i = 0; i < slices; ++i) {
        unsigned int curr = i * 2;      // Current pair start
        unsigned int next = (i + 1) * 2; // Next pair start
        
        // Triangle 1: curr_top, curr_bottom, next_top
        indices.push_back(curr);        // Current top
        indices.push_back(curr + 1);    // Current bottom
        indices.push_back(next);        // Next top
        
        // Triangle 2: curr_bottom, next_bottom, next_top  
        indices.push_back(curr + 1);    // Current bottom
        indices.push_back(next + 1);    // Next bottom
        indices.push_back(next);        // Next top
    }
}
};

#endif // SHAPE_H
