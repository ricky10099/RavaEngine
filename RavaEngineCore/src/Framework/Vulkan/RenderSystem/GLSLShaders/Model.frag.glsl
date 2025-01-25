#version 450
#pragma shader_stage(fragment)
#extension GL_KHR_vulkan_glsl: enable

#include "../../GPUSharedDefines.h"

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec3 fragPosition;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec2 fragUV;
layout (location = 4) in vec3 fragTangent;

layout (set = 1, binding = 1) uniform sampler2D diffuseMap;
layout (set = 1, binding = 2) uniform sampler2D normalMap;
layout (set = 1, binding = 3) uniform sampler2D roughnessMetallicMap;
layout (set = 1, binding = 4) uniform sampler2D emissiveMap;
layout (set = 1, binding = 5) uniform sampler2D roughnessMap;
layout (set = 1, binding = 6) uniform sampler2D metallicMap;

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

layout (set = 1, binding = 0) uniform MaterialUbo {
    int features;
    float roughness;
    float metallic;
    float spare0; // padding

    // byte 16 to 31
    vec4 diffuseColor;

    // byte 32 to 47
    vec3 emissiveColor;
    float emissiveStrength;

    // byte 48 to 63
    float normalMapIntensity;
    float spare1; // padding
    float spare2; // padding
    float spare3; // padding

    // byte 64 to 128
    vec4 spare4[4];
}matUbo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const float PI = 3.14159265359;

vec3 Uncharted2Tonemap(vec3 x) {
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2(vec3 color) {
  const float W = 11.2;
  float exposureBias = 2.0;
  vec3 curr = Uncharted2Tonemap(exposureBias * color);
  vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
  return curr * whiteScale;
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float Rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec3 ambientLightColor = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);

    vec3 surfaceNormal = normalize(fragNormal);
    vec3 cameraPosWorld = ubo.view[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosition);
    
    // diffuse
    vec4 diffuseColor;
    if(bool(matUbo.features & GLSL_HAS_DIFFUSE_MAP)) {
        diffuseColor = texture(diffuseMap, fragUV) * matUbo.diffuseColor;
    }else{
        diffuseColor = fragColor;
    }
    if(diffuseColor.a < 0.5) {
        discard;
    }


    // normal
    vec4 normal;
    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent);
    // Gram Schmidt
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    float normalMapIntensity  = matUbo.normalMapIntensity;
    vec3 normalTangentSpace;
    if (bool(matUbo.features & GLSL_HAS_NORMAL_MAP)) {
        normalTangentSpace = texture(normalMap,fragUV).xyz * 2 - vec3(1.0, 1.0, 1.0);
        normalTangentSpace = mix(vec3(0.0, 0.0, 1.0), normalTangentSpace, normalMapIntensity);
        surfaceNormal = normalize(TBN * normalTangentSpace);
        normal = vec4(surfaceNormal, 1.0);
    } else {
        surfaceNormal = N;
        normal = vec4(N, 1.0);
    }


    // roughness, metallic
    float roughness;
    float metallic;
    if (bool(matUbo.features & GLSL_HAS_ROUGHNESS_METALLIC_MAP)) {
        roughness = texture(roughnessMetallicMap, fragUV).g;
        metallic = texture(roughnessMetallicMap, fragUV).b;
    } else {
        if (bool(matUbo.features & GLSL_HAS_ROUGHNESS_MAP)) {
            roughness = texture(roughnessMap, fragUV).r; // gray scale
        } else {
            roughness = matUbo.roughness;
        }
        if (bool(matUbo.features & GLSL_HAS_METALLIC_MAP)) {
            metallic = texture(metallicMap, fragUV).r; // gray scale
        }
        else {
            metallic = matUbo.metallic;
        }
    }
    vec4 material = vec4(normalMapIntensity, roughness, metallic, 0.0);

    // emissive material
    vec4 emissive;
    vec4 emissiveColor = vec4(matUbo.emissiveColor.r, matUbo.emissiveColor.g, matUbo.emissiveColor.b, 1.0);
    if (bool(matUbo.features & GLSL_HAS_EMISSIVE_MAP)) {
        vec4 fragEmissiveColor = texture(emissiveMap, fragUV);        
        emissive = fragEmissiveColor * emissiveColor * matUbo.emissiveStrength;
    } else {
        emissive = emissiveColor * matUbo.emissiveStrength;
    }

    vec3 camPos = (inverse(ubo.view) * vec4(0.0,0.0,0.0,1.0)).xyz;

    N = normalize(surfaceNormal);
    vec3 V = normalize(camPos - fragPosition);

    vec3 fragColor = diffuseColor.rgb;
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, fragColor, metallic);
    // reflectance equation
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        // calculate per-light radiance
        vec3 L = normalize(light.position.xyz - fragPosition);
        vec3 H = normalize(V + L);
        float directionToLight = length(light.position.xyz - fragPosition);
        float attenuation = 1.0 / (directionToLight * directionToLight);
        float lightIntensity = light.color.w;
        vec3 radiance = light.color.rgb * lightIntensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);  
        vec3 F    = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * fragColor / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    {
        // calculate radiance for a directional light
        vec3 L = normalize(-ubo.directionalLight.direction.xyz);
        vec3 H = normalize(V + L);
        float lightIntensity = ubo.directionalLight.color.w;
        vec3 radiance = ubo.directionalLight.color.rgb * lightIntensity;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);  
        vec3 F    = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);
        float litPercentage = 1.0;

        // add to outgoing radiance Lo
        Lo += (kD * fragColor / PI + specular) * radiance * NdotL * litPercentage;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    vec3 color = ambientLightColor + Lo;
    color = Uncharted2(color);

    // Set Gamma as 2.0, may set it changable in future
	color = pow(color, vec3(1.0 / 2.0));

    outColor = diffuseColor * vec4(color, 1.0);
}
