#version 450


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 postion;
 
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPos;

void main() {
    outColor = vec4(fragColor , 1.0);
    outNormal = vec4(normal,0.4);
    outPos = vec4(postion,0.5);
}