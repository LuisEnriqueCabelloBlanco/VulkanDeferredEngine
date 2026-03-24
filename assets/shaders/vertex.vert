#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant, std430) uniform pc{
    mat4 modelMat;
};

layout(location = 0) in vec3 inPosition;

layout(location = 1) in vec3 inColor;

layout(location = 2) in vec2 inTexCoord;

layout(location = 3) in vec3 inNormal;

layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;

layout(location = 1) out vec2 fragTexCoord;

layout(location = 2) out vec3 normal;

layout(location = 3) out vec3 outPosition;

layout(location = 4) out mat3 TBN;

void main() {
   // ubo.proj * ubo.view * ubo.model *
   gl_Position = ubo.proj * ubo.view * modelMat * vec4(inPosition, 1.0);
   outPosition = (modelMat * vec4(inPosition, 1.0)).rgb;
   fragColor = inColor;
   fragTexCoord = inTexCoord;
   normal = normalize(modelMat*vec4(inNormal,0)).rgb;
   vec3 tangent = normalize( vec3(modelMat * vec4(inTangent, 0)));

   vec3 bitangent = normalize(cross(normal,tangent));
   TBN = mat3(tangent,bitangent,normal);
}