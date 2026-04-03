#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant, std430) uniform pc{
    mat4 modelMat;
};

layout(location = 0) in vec3 inPosition;

void main() {
    vec4 modelPos =  modelMat * vec4(inPosition, 1.0);
   gl_Position = ubo.proj * ubo.view * modelPos;
}