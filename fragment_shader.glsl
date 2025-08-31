#version 330 core

in vec4 vColor;        // interpolated color from vertex shader
out vec4 FragColor;    // final fragment color

void main()
{
    FragColor = vColor;
}
