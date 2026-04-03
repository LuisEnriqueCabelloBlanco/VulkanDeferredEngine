#version 450
#extension GL_EXT_nonuniform_qualifier : enable

struct MaterialParams{
    float metallic;
    float roughness;
    int textureIdx;
    int normalIdx;
};

layout(push_constant, std430) uniform pc_i{
    layout(offset = 64) MaterialParams mat;
};

layout(set = 1, binding  = 0) uniform sampler2D texSampler[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 postion;
layout(location = 4) in mat3 TBN;
 
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPos;


void main() {

    outColor = texture(texSampler[mat.textureIdx],fragTexCoord)*vec4(fragColor , 1.0);

    vec3 sampleNormal = normal;

    if(mat.normalIdx!=-1){
        sampleNormal = TBN * texture(texSampler[mat.normalIdx],fragTexCoord).rgb;
    }

    outNormal = vec4(sampleNormal,mat.roughness);
    outPos = vec4(postion,mat.metallic);
}