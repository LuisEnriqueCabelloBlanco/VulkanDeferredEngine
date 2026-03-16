#version 450

layout (input_attachment_index = 0, set = 0, binding = 2) uniform subpassInput color;
layout (input_attachment_index = 1, set = 0, binding = 3) uniform subpassInput normal;


//layout(binding  = 1) uniform sampler2D texSampler;
layout(binding = 4 ) uniform GlobalLightData{
    vec3 lightDir;
    float ambient;
    vec4 lightColor;
}light;

layout(location = 0) out vec4 outColor;


void main() {
    vec4 diff = light.lightColor * max(0,dot(subpassLoad(normal).rgb,-light.lightDir));
    vec4 amb = light.ambient * light.lightColor;
    outColor = subpassLoad(color)*(amb + diff);
}