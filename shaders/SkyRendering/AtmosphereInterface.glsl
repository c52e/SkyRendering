#ifndef __ATMOSPHERE_INTERFACE_GLSL
#define __ATMOSPHERE_INTERFACE_GLSL

#include "Atmosphere.glsl"

bool FromSpaceIntersectTopAtmosphereBoundary(float r, float mu, out float near_distance) {
    float discriminant = r * r * (mu * mu - 1.0) + top_radius * top_radius;
    if (mu < 0.0 && discriminant >= 0.0) {
        near_distance = ClampDistance(-r * mu - SafeSqrt(discriminant));
        return true;
    }
    return false;
}

vec3 GetTextureCoordFromUnitRange(vec3 xyz, ivec3 texture_size) {
    return 0.5 / vec3(texture_size) + xyz * (1.0 - 1.0 / vec3(texture_size));
}

vec3 GetAerialPerspectiveTextureUvwFromTexCoordDistance(vec2 TexCoord, float marching_distance
    , float aerial_perspective_lut_max_distance, ivec3 aerial_perspective_lut_size) {
    return GetTextureCoordFromUnitRange(vec3(TexCoord,
        sqrt(marching_distance / aerial_perspective_lut_max_distance)), aerial_perspective_lut_size);
}

#endif
