#version 450
#pragma shader_stage(vertex)
#extension GL_KHR_vulkan_glsl: enable

#include "../../GPUSharedDefines.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = pushConstants.transformMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
}