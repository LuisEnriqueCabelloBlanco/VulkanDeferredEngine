#version 450
#define M_PI 3.1415926535897932384626433832795

layout (input_attachment_index = 0, set = 0, binding = 2) uniform subpassInput color;
layout (input_attachment_index = 1, set = 0, binding = 3) uniform subpassInput normal;
layout (input_attachment_index = 2, set = 0, binding = 5) uniform subpassInput position;


struct PointLight{
    vec4 color;
    vec3 position;
    float intensity;
};

struct DirectionalLight{
    vec4 color;
    vec3 direction;
    float intensity;
};

struct Light{
    int type; // 0 direcional, 1 puntual
    vec3 dir; // direccion de la luz (direcioal)
    vec3 center; // posicon de la luz
    float intensity; // intensidad
    vec3 color; // color
};

//layout(binding  = 1) uniform sampler2D texSampler;
layout(binding = 4 ) uniform GlobalLightData{
    DirectionalLight dirLight;
    PointLight pointLight;
    vec3 eyePos;
    float ambient;
}light;

layout(location = 0) out vec4 outColor;



float DistributionGGX (vec3 norm, vec3 halfView, float roughness){
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(0, dot(norm, halfView));
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 -1.0) + 1.0);

    denom = M_PI * denom*denom;

    return nom/denom;
}


float GeometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness+1.0);
    float k = (r*r)/8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0-k) + k;

    return nom/denom;
}


float GeometrySmith(vec3 normalVec, vec3 view, vec3 lightVec, float roughness){
    float NdotV = max(0, dot(normalVec, view));
    float NdotL = max(0, dot(normalVec, lightVec));

    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx2*ggx1;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0)*pow((1.0 + 0.000001/*avoid negative approximation when cosTheta = 1*/) - cosTheta, 5.0);
}

//asumimos que los datos que nos pasan son normalizados
vec3 BRDF(vec3 normalVec, vec3 viewVec, vec3 baseFresnel, vec3 albedo, vec3 halfView, Light l, float metallic, float roughness){
    vec3 radiance = l.color * l.intensity;

    float NDF = DistributionGGX(normalVec, halfView,roughness);
    float G = GeometrySmith(normalVec,viewVec,l.dir,roughness);
    vec3 F = fresnelSchlick(max(0,dot(halfView,l.dir)), baseFresnel);


    vec3 kS = F;
    vec3 kD = vec3(1)-kS;
    kD*=1.0-metallic;

    vec3 nom = NDF * G * F;
    float denom = 4 * max(0,dot(normalVec,viewVec)) * max(0,dot(normalVec,l.dir))+0.0001;
    vec3 specular = nom / denom;


    float NdotL = max(0, dot(normalVec,l.dir));

    vec3 diffuse_radiance = kD * (albedo)*M_PI;

    return (diffuse_radiance + specular)*radiance*NdotL;
}

//asumimos que la intesidad de la luz que nos lllega ya esta atenuada
vec3 PBR(Light l, float ambient, vec3 albedo, float metallic, float roughness, vec3 normalVec, vec3 view ){

    vec3 F0 = vec3(0.03);

    F0 = mix(F0,albedo, metallic);

    vec3 halfView = normalize(view+l.dir);

    return ambient*albedo + BRDF(normalVec,view,F0,albedo, halfView, l,metallic, roughness );
}

void main() {

    float roughness = subpassLoad(normal).a;

    float metallic = subpassLoad(position).a;

    vec3 normalVec = normalize(subpassLoad(normal).rgb);

    vec3 viewVector =normalize( light.eyePos-subpassLoad(position).rgb);
    vec3 sampleColor = subpassLoad(color).rgb;



    vec3 colorVal = vec3(0);
    
    Light lightData;

    lightData.dir = -normalize(light.dirLight.direction);
    lightData.color = light.dirLight.color.rgb;
    lightData.intensity = light.dirLight.intensity;
    
    
    
    colorVal += PBR(lightData, light.ambient, sampleColor, metallic, roughness, normalVec, viewVector);

    vec3 ponintToPos = light.pointLight.position-subpassLoad(position).rgb;
    //color += BRDF(light.pointLight.intensity,ponintDir,viewVector,normalVec,roughness);

    lightData.dir = normalize(ponintToPos);
    lightData.color = light.pointLight.color.rgb;
    lightData.intensity = light.dirLight.intensity/(length(ponintToPos)*length(ponintToPos));

    colorVal += PBR(lightData, light.ambient, sampleColor, metallic, roughness, normalVec, viewVector);

    outColor = vec4(colorVal,1);
    //outColor = subpassLoad(position);
}