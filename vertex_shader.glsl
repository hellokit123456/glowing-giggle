#version 330 core

layout(location = 0) in vec4 aPos;    // vertex position (from VBO)
layout(location = 1) in vec4 aColor;  // vertex color (from VBO)

out vec4 vColor;   // passed to fragment shader

uniform mat4 uMVP; // Model-View-Projection matrix

void main()
{
    gl_Position = uMVP * aPos;
    vColor = aColor;
}
