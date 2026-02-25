#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput color;
//layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput normal;


//layout(binding  = 1) uniform sampler2D texSampler;


layout(location = 0) out vec4 outColor;


void main() {
    outColor = subpassLoad(color);
}