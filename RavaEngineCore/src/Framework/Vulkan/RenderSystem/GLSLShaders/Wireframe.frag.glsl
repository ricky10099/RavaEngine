#version 450
#pragma shader_stage(fragment)
#extension GL_KHR_vulkan_glsl: enable

#include "../../GPUSharedDefines.h"

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 0.5); // Semi-transparent wireframe
}