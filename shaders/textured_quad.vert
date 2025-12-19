#version 450


layout(location = 0) in vec2 inPosition;  // x, y
layout(location = 1) in vec2 inTexCoord;  // u, v


layout(location = 0) out vec2 outTexCoord;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    outTexCoord = inTexCoord;
}