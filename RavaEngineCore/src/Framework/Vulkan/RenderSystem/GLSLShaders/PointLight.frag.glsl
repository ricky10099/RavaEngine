#version 450
#pragma shader_stage(fragment)
#extension GL_KHR_vulkan_glsl: enable

#include "../../GPUSharedDefines.h"

layout (location = 0) in vec2 fragOffset;
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

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;

const float M_PI = 3.1415926538;

void main() {
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0) {
        discard;
    }

    float oneMinusDisSqr = (1 - dis) * (1 - dis);
    vec3 bloom = vec3(oneMinusDisSqr, oneMinusDisSqr, oneMinusDisSqr);
    float alpha = smoothstep(0.1, 1.0, 1-dis);
    outColor = vec4(push.color.xyz + bloom, alpha);

//  float cosDis = 0.5 * (cos(dis * M_PI) + 1.0); // ranges from 1 -> 0
//  outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis);
}
