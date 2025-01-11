#version 450
#pragma shader_stage(vertex)
#extension GL_KHR_vulkan_glsl: enable

#include "../../GPUSharedDefines.h"

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in ivec4 jointIds;
layout(location = 6) in vec4 weights;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec3 fragTangent;

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

//layout(set = 2, binding = 0) uniform SkeletonUbo {
//	mat4 finalJointsMatrices[MAX_JOINTS];
//} skeletonUbo;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 normalMatrix;
} push;

//const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 3.0, 1.0));

void main() {
  vec4 animatedPosition = vec4(position, 1.0f);
  vec3 weightColor = vec3(0.0); // Initialize the color to black
//  vec4 animatedPosition = vec4(1.0f);
    mat4 jointTransform = mat4(1.0f);
//    for (int i = 0; i < MAX_JOINT_INFLUENCE; ++i){
//        if (weights[i] == 0)
//            continue;
//        if (jointIds[i] >=MAX_JOINTS) {
//            animatedPosition = vec4(position,1.0f);
//            jointTransform = mat4(1.0f);
//            break;
//        }
//        weightColor += weights[i] * vec3(float(i) / MAX_JOINT_INFLUENCE, 0.5, 1.0 - float(i) / MAX_JOINT_INFLUENCE);
//        vec4 localPosition  = skeletonUbo.finalJointsMatrices[jointIds[i]] * vec4(position,1.0f);
//        animatedPosition += localPosition * weights[i];
//        jointTransform += skeletonUbo.finalJointsMatrices[jointIds[i]] * weights[i];
//    }

//  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
  vec4 positionWorld = push.modelMatrix * animatedPosition;
  gl_Position = ubo.projection * ubo.view * positionWorld;
//  fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
  fragPosWorld = positionWorld.xyz;

  mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix) * mat3(jointTransform)));
  vec3 normalWorldSpace = normalize(normalMatrix * normal);
    fragNormalWorld = normalWorldSpace;
    fragTangent = normalize(normalMatrix * tangent);

//    float lightIntensity = max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0);
//
//    fragColor = lightIntensity * color;
//fragColor = vec4(weightColor, 1.0);  Output the color based on weights
    fragColor = color;
    fragUV = uv;
}
