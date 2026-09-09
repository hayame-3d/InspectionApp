#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 v_Color;

uniform mat4 u_PV;

void main() {
    gl_Position = u_PV * vec4(aPos, 1.0);
    v_Color = aColor;
}
