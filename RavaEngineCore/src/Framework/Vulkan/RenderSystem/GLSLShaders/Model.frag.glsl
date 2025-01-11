#version 450
#pragma shader_stage(fragment)
#extension GL_KHR_vulkan_glsl: enable

#include "../../GPUSharedDefines.h"

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragUV;
layout (location = 4) in vec3 fragTangent;

layout (set = 1, binding = 1) uniform sampler2D diffuseMap;

layout (location = 0) out vec4 outColor;

struct PointLight {
    vec4 position; // ignore w
    vec4 color; // w is intensity
};

struct DirectionalLight {
    vec4 direction;  // ignore w
    vec4 color;     // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLights[MAX_LIGHTS];
    DirectionalLight directionalLight;
    int numLights;
} ubo;

//layout (set = 1, binding = 0) uniform MaterialUbo {
//    int features;
//    float roughness;
//    float metallic;
//    float spare0; // padding
//
//    // byte 16 to 31
//    vec4 diffuseColor;
//
//    // byte 32 to 47
//    vec3 emissiveColor;
//    float emissiveStrength;
//
//    // byte 48 to 63
//    float normalMapIntensity;
//    float spare1; // padding
//    float spare2; // padding
//    float spare3; // padding
//
//    // byte 64 to 128
//    vec4 spare4[4];
//}matUbo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, -3.0, -1.0));
const vec3 DIRECTIONAL_LIGHT_COLOR = vec3(1.0, 1.0, 1.0);

void main() {
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.view[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    vec4 textureColor;
//    if(bool(matUbo.features & GLSL_HAS_DIFFUSE_MAP)) {
//        textureColor = texture(diffuseMap, fragUV) * matUbo.diffuseColor;
//    }else{
        textureColor = fragColor;
//    }
//    if(textureColor.a < 0.5) {
//        discard;
//    }
//

    vec3 directionalDiffuse;
    vec3 directionalSpecular;

    if(true){
        // Calculate directional light contribution
        float cosAngIncidenceDir = max(dot(surfaceNormal, normalize(-ubo.directionalLight.direction.xyz)), 0.0);
        directionalDiffuse = ubo.directionalLight.color.xyz * cosAngIncidenceDir;
    
        // Specular for directional light
        vec3 halfAngleDir = normalize(-ubo.directionalLight.direction.xyz + viewDirection);
        float blinnTermDir = pow(max(dot(surfaceNormal, halfAngleDir), 0.0), 512.0);
        directionalSpecular = ubo.directionalLight.color.xyz * blinnTermDir;
    }
    else{
        // Calculate directional light contribution
        float cosAngIncidenceDir = max(dot(surfaceNormal, -DIRECTION_TO_LIGHT), 0.0);
        directionalDiffuse = DIRECTIONAL_LIGHT_COLOR * cosAngIncidenceDir;

        // Specular for directional light
        vec3 halfAngleDir = normalize(-DIRECTION_TO_LIGHT + viewDirection);
        float blinnTermDir = pow(max(dot(surfaceNormal, halfAngleDir), 0.0), 512.0);
        directionalSpecular = DIRECTIONAL_LIGHT_COLOR * blinnTermDir;
    }

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        // specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
        specularLight += intensity * blinnTerm;
    }
  
    diffuseLight += directionalDiffuse;
    specularLight += directionalSpecular;

    outColor = vec4(diffuseLight, 1.0) * textureColor + vec4(specularLight, 1.0);
}
