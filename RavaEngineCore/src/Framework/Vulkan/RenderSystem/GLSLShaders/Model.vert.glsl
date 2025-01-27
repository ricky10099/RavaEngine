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
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec3 fragTangent;

struct PointLight {
    vec4 position;  // ignore w
    vec4 color;     // w is intensity
};

struct DirectionalLight {
    vec4 direction;  // ignore w
    vec4 color;      // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;  // w is intensity
    PointLight pointLights[MAX_LIGHTS];
    DirectionalLight directionalLight;
    int numLights;
    float gamma;
	float exposure;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;


void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0f);
    gl_Position = ubo.projection * ubo.view * positionWorld;
    fragPosition = positionWorld.xyz;

    fragNormal = normalize(mat3(push.normalMatrix) * normal);
    fragTangent = normalize(mat3(push.normalMatrix) * tangent);
    fragColor = color;
    fragUV = uv;
}
