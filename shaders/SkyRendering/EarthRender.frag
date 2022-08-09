#version 460
#include "Atmosphere.glsl"
layout(binding = 0) uniform sampler2D depth_stencil_texture;
layout(binding = 1) uniform sampler2D earth_albedo;

layout(std140, binding = 1) uniform AtmosphereRenderBufferData{
    mat4 view_projection;
    mat4 inv_view_projection;
    vec3 camera_position;
    float camera_earth_center_distance;
    vec3 earth_center;
    float padding;
    vec3 up_direction;
    float padding1;
};

in vec2 vTexCoord;
layout(location = 0) out vec4 Albedo;
layout(location = 1) out vec4 Normal;
layout(location = 2) out vec4 ORM;

vec3 GetEarthAlbedo(sampler2D earth_albedo, vec3 ground_position) {
    vec3 direction = normalize(ground_position - earth_center);
    float theta = acos(direction.y);
    float phi = atan(direction.x, direction.z);
    vec2 coord = vec2(INV_PI * 0.5 * phi + 0.5, 1.0 - theta * INV_PI);
#if 0
    vec3 color = texture(earth_albedo, coord).rgb;
#else
    // Make the Earth seamless
    vec2 dudxy1 = vec2(dFdx(coord.x), dFdy(coord.x));
    vec2 dudxy2 = vec2(dFdx(fract(coord.x + 0.5)), dFdy(fract(coord.x + 0.5)));
    vec2 dudxy = length(dudxy1) < length(dudxy2) ? dudxy1 : dudxy2;
    vec2 dvdxy = vec2(dFdx(coord.y), dFdy(coord.y));
    vec3 color = textureGrad(earth_albedo, coord, vec2(dudxy.x, dvdxy.x), vec2(dudxy.y, dvdxy.y)).rgb;
#endif
    return pow(color, vec3(2.2));
}

void main() {
    float depth = texelFetch(depth_stencil_texture, ivec2(gl_FragCoord.xy), 0).x;
    vec3 fragment_position = ProjectiveMul(inv_view_projection, vec3(vTexCoord, depth) * 2.0 - 1.0);
    vec3 view_direction = normalize(fragment_position - camera_position);
    float r = camera_earth_center_distance;
    float mu = dot(view_direction, up_direction);
    if (!RayIntersectsGround(r, mu))
        discard;
    float dist = DistanceToBottomAtmosphereBoundary(r, mu);
    if (dist >= distance(fragment_position, camera_position))
        discard;
    vec3 ground_position = camera_position + view_direction * dist;
    gl_FragDepth = ProjectiveMul(view_projection, ground_position).z * 0.5 + 0.5;
    vec3 albedo = GetEarthAlbedo(earth_albedo, ground_position);
    Albedo = vec4(albedo, 1.0);
    vec3 normal = normalize(ground_position - earth_center);
    Normal = vec4(normal, 1.0);
    float metallic = 0.0;
    float roughness = 1.0;
    ORM = vec4(1.0, roughness, metallic, 1.0);
}
