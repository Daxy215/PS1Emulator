#version 330 core

in vec3 color;
flat in uvec2 attr;

out vec4 fragColor;

void main() {
    float alpha = (float(attr.x) == 1) ? 0.5 : 1.0;
    fragColor = vec4(color, alpha);
}