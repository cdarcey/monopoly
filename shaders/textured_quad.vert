#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(set = 3, binding = 0) uniform DynamicData {
    mat4 mvp;
} uDynamic;

layout(location = 0) out vec2 outUV;

void main() {
    gl_Position = uDynamic.mvp * vec4(inPosition, 0.0, 1.0);
    outUV = vec2(inUV.x, 1.0 - inUV.y);  // flip V coordinate
}