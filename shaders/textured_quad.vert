
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(set = 3, binding = 0) uniform pl_dynamic_block
{
    float tScalerValue;
} tDynamicBlock;

layout(location = 0) out vec2 outUV;

void main() {
    gl_Position = vec4(inPosition * tDynamicBlock.tScalerValue, 0.0, 1.0);
    outUV = inUV;
}