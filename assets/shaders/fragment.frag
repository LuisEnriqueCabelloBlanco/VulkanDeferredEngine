#version 450


struct MaterialParams{
    float metallic;
    float roughness;
    int textureIdx;
};

layout(push_constant, std430) uniform pc_i{
    layout(offset = 64) MaterialParams mat;
};

layout(binding  = 1) uniform sampler2D texSampler[2];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 postion;
 
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPos;


void main() {

    int a =2;
    outColor = texture(texSampler[mat.textureIdx],fragTexCoord)*vec4(fragColor , 1.0);
    outNormal = vec4(normal,mat.roughness);
    outPos = vec4(postion,mat.metallic);
}