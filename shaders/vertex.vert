#version 450

layout(set = 0,binding = 0) uniform UniformBufferObject {
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
   vec4 modelPos = modelMat * vec4(inPosition, 1.0);
   gl_Position = ubo.proj * ubo.view * modelPos;
   outPosition = (modelMat * vec4(inPosition, 1.0)).rgb;
   fragColor = inColor;
   fragTexCoord = inTexCoord;

    vec3 n = normalize((modelMat * vec4(inNormal, 0.0)).xyz);
    vec3 t = normalize((modelMat * vec4(inTangent, 0.0)).xyz);

    // Gram-Schmidt: mantenemos la tangente ortogonal a la normal.
    t = normalize(t - n * dot(t, n));
    vec3 b = normalize(cross(n, t));

    normal = n;
    TBN = mat3(t, b, n);
}
