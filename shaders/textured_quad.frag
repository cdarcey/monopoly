#version 450

layout(location = 0) in vec2 inTexCoord;


layout(set = 0, binding = 0) uniform texture2D texImage;
layout(set = 0, binding = 1) uniform sampler texSampler;


layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(texImage, texSampler), inTexCoord);
}