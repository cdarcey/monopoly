
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform texture2D texSampler;
layout(set = 0, binding = 1) uniform sampler samp;


layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(texSampler, samp), inUV);
}