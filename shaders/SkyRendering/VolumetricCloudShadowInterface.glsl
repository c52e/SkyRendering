#ifndef _VOLUMETRIC_CLOUD_SHADOW_INTERFACE_GLSL
#define _VOLUMETRIC_CLOUD_SHADOW_INTERFACE_GLSL

float SampleCloudShadowTransmittance(sampler2D cloud_shadow_map, vec3 light_ndc) {
    const float kInvTransitionDepth = 1.0 / 0.5;
    vec2 depth_transmittance = texture(cloud_shadow_map, light_ndc.xy * 0.5 + 0.5).xy;
    return mix(depth_transmittance.g, 1.0, clamp((depth_transmittance.r - light_ndc.z) * kInvTransitionDepth, 0, 1));
}

float SampleRayScatterVisibility(sampler3D shadow_froxel, vec2 uv, float dist, float inv_max_dist) {
    float w = dist * inv_max_dist;
    return mix(1.0, texture(shadow_froxel, vec3(uv, w)).r, clamp(1.0 / w, 0, 1));
}

#endif
