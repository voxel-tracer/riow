#version 330 core
out vec4 FragColor;
in vec3 oColor;

void main() {
    FragColor = vec4(oColor, 1.0);
}