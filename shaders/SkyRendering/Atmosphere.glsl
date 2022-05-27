#ifndef _ATMOSPHERE_GLSL
#define _ATMOSPHERE_GLSL

#include "Common.glsl"

layout(std140, binding = 0) uniform AtmosphereBufferData{
    vec3 solar_illuminance;
    float sun_angular_radius;

    vec3 rayleigh_scattering;
    float inv_rayleigh_exponential_distribution;

    vec3 mie_scattering;
    float inv_mie_exponential_distribution;

    vec3 mie_absorption;
    float ozone_center_altitude;

    vec3 ozone_absorption;
    float inv_ozone_width;

    vec3 ground_albedo;
    float mie_phase_g;

    vec3 _atmosphere_padding;
    float multiscattering_mask;

    float bottom_radius;
    float top_radius;
    float transmittance_steps;
    float multiscattering_steps;
};

// https://ebruneton.github.io/precomputed_atmospheric_scattering/
// https://github.com/sebh/UnrealEngineSkyAtmosphere

float ClampCosine(float mu) {
    return clamp(mu, -1.0, 1.0);
}

float ClampDistance(float d) {
    return max(d, 0.0);
}

float ClampRadius(float r) {
    return clamp(r, bottom_radius, top_radius);
}

float SafeSqrt(float a) {
    return sqrt(max(a, 0.0));
}

vec2 GetTextureCoordFromUnitRange(vec2 xy, ivec2 texture_size) {
    return 0.5 / vec2(texture_size) + xy * (1.0 - 1.0 / vec2(texture_size));
}

bool RayIntersectsGround(float r, float mu) {
    return mu < 0.0 && r * r * (mu * mu - 1.0) + bottom_radius * bottom_radius >= 0.0;
}

float DistanceToTopAtmosphereBoundary(float r, float mu) {
    float discriminant = r * r * (mu * mu - 1.0) + top_radius * top_radius;
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

float DistanceToBottomAtmosphereBoundary(float r, float mu) {
    float discriminant = r * r * (mu * mu - 1.0) + bottom_radius * bottom_radius;
    return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

void GetRMuFromTransmittanceTextureIndex(ivec2 index, ivec2 size, out float r, out float mu) {
    vec2 uv = vec2(index) / vec2(size - 1);
    float x_mu = uv.x;
    float x_r = uv.y;
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(top_radius * top_radius - bottom_radius * bottom_radius);
    // Distance to the horizon, from which we can compute r:
    float rho = H * x_r;
    r = sqrt(rho * rho + bottom_radius * bottom_radius);
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
    // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon) -
    // from which we can recover mu:
    float d_min = top_radius - r;
    float d_max = rho + H;
    float d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * r * d);
    mu = ClampCosine(mu);
}

vec2 GetTransmittanceTextureUvFromRMu(ivec2 size, float r, float mu) {
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(top_radius * top_radius - bottom_radius * bottom_radius);
    // Distance to the horizon.
    float rho = SafeSqrt(r * r - bottom_radius * bottom_radius);
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
    // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
    float d = DistanceToTopAtmosphereBoundary(r, mu);
    float d_min = top_radius - r;
    float d_max = rho + H;
    float x_mu = (d - d_min) / (d_max - d_min);
    float x_r = rho / H;
    return GetTextureCoordFromUnitRange(vec2(x_mu, x_r), size);
}

vec3 GetTransmittanceToTopAtmosphereBoundary(sampler2D transmittance_texture, float r, float mu) {
    vec2 uv = GetTransmittanceTextureUvFromRMu(textureSize(transmittance_texture, 0), r, mu);
    return texture(transmittance_texture, uv).rgb;
}

vec3 GetSunVisibility(sampler2D transmittance_texture, float r, float mu_s) {
    float sin_theta_h = bottom_radius / r;
    float cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
    return GetTransmittanceToTopAtmosphereBoundary(transmittance_texture, r, mu_s) *
        smoothstep(-sin_theta_h * sun_angular_radius,
            sin_theta_h * sun_angular_radius,
            mu_s - cos_theta_h);
}

vec3 GetExtinction(float altitude) {
    vec3 rayleigh_extinction = rayleigh_scattering *
        clamp(exp(-altitude * inv_rayleigh_exponential_distribution), 0, 1);

    vec3 mie_extinction = (mie_scattering + mie_absorption) *
        clamp(exp(-altitude * inv_mie_exponential_distribution), 0, 1);

    vec3 ozone_extinction = ozone_absorption *
        max(0, altitude < ozone_center_altitude ?
            1 + (altitude - ozone_center_altitude) * inv_ozone_width :
            1 - (altitude - ozone_center_altitude) * inv_ozone_width);

    return rayleigh_extinction + mie_extinction + ozone_extinction;
}

float IsotropicPhaseFunction() {
    return 1.0 / (4.0 * PI);
}

float RayleighPhaseFunction(float cos_theta) {
#ifndef MULTISCATTERING_COMPUTE_PROGRAM
    float k = 3.0 / (16.0 * PI);
    return k * (1.0 + cos_theta * cos_theta);
#else
    return IsotropicPhaseFunction();
#endif
}

float MiePhaseFunction(float g, float cos_theta) {
#ifndef MULTISCATTERING_COMPUTE_PROGRAM
    float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + cos_theta * cos_theta) / pow(1.0 + g * g - 2.0 * g * cos_theta, 1.5);
#else
    return IsotropicPhaseFunction();
#endif
}

void GetScattering(float altitude, out vec3 rayleigh, out vec3 mie) {
    rayleigh = rayleigh_scattering * clamp(exp(-altitude * inv_rayleigh_exponential_distribution), 0, 1);
    mie = mie_scattering * clamp(exp(-altitude * inv_mie_exponential_distribution), 0, 1);
}

void GetAltitudeMuSFromMultiscatteringTextureIndex(ivec2 index, ivec2 size, out float altitude, out float mu_s) {
    vec2 uv = vec2(index) / vec2(size - 1);
    float x_mu_s = uv.x;
    float x_altitude = uv.y;
    altitude = x_altitude * (top_radius - bottom_radius);
    mu_s = x_mu_s * 2.0 - 1.0;
}

vec2 GetMultiscatteringTextureUvFromRMuS(ivec2 size, float r, float mu_s) {
    float x_mu_s = mu_s * 0.5 + 0.5;
    float x_r = (r - bottom_radius) / (top_radius - bottom_radius);
    return GetTextureCoordFromUnitRange(vec2(x_mu_s, x_r), size);
}

vec3 GetMultiscatteringContribution(sampler2D multiscattering_texture, float r, float mu_s) {
    vec2 uv = GetMultiscatteringTextureUvFromRMuS(textureSize(multiscattering_texture, 0), r, mu_s);
    return texture(multiscattering_texture, uv).rgb;
}

float GetVisibilityFromShadowMap(sampler2DShadow shadow_map, mat4 light_view_projection, vec3 position) {
    vec4 xyzw = light_view_projection * vec4(position, 1.0);
    vec3 xyz = xyzw.xyz / xyzw.w;
    xyz = xyz * 0.5 + 0.5;
    vec2 shadow_map_coord = xyz.xy;
    float depth = xyz.z;
    if (depth >= 1.0) return 1.0;
    return texture(shadow_map, vec3(shadow_map_coord, depth));
}

float GetVisibilityFromMoonShadow(float sun_moon_angular_distance,
        float sun_angular_radius, float moon_angular_radius) {
    // 可以用一个预计算的二维纹理加速
    float max_radius = sun_angular_radius + moon_angular_radius;
    float min_radius = abs(sun_angular_radius - moon_angular_radius);
    float sun_r2 = sun_angular_radius * sun_angular_radius;
    float moon_r2 = moon_angular_radius * moon_angular_radius;
    if (sun_moon_angular_distance >= max_radius)
        return 1.0;
    if (sun_moon_angular_distance <= min_radius)
        return clamp((sun_r2 - moon_r2) / sun_r2, 0.0, 1.0);
    float distance2 = sun_moon_angular_distance * sun_moon_angular_distance;
    float cos_half_sun = (distance2 + sun_r2 - moon_r2) / (2 * sun_moon_angular_distance * sun_angular_radius);
    float cos_half_moon = (distance2 + moon_r2 - sun_r2) / (2 * sun_moon_angular_distance * moon_angular_radius);
    float half_sun = acos(cos_half_sun);
    float half_moon = acos(cos_half_moon);
    float triangle_h = sun_angular_radius * sqrt(1 - cos_half_sun * cos_half_sun);
    float area_total = (PI - half_sun) * sun_r2 + (PI - half_moon) * moon_r2 + triangle_h * sun_moon_angular_distance;
    float area_uncovered = area_total - PI * moon_r2;
    return area_uncovered / (PI * sun_r2);
}

float GetVisibilityFromMoonShadow(vec3 moon_vector, float moon_radius, vec3 sun_direction) {
    float inv_moon_distance = 1.0 / length(moon_vector);
    vec3 moon_direction = moon_vector * inv_moon_distance;
    float moon_angular_radius = asin(clamp(moon_radius * inv_moon_distance, -1, 1));
    return GetVisibilityFromMoonShadow(acos(clamp(dot(sun_direction, moon_direction), -1, 1)),
        sun_angular_radius, moon_angular_radius);
}

vec3 ComputeScatteredLuminance(sampler2D transmittance_texture
#ifndef MULTISCATTERING_COMPUTE_PROGRAM
    , sampler2D multiscattering_texture
#if VOLUMETRIC_LIGHT_ENABLE
    , sampler2DShadow shadow_map
    , mat4 light_view_projection
#endif
#if MOON_SHADOW_ENABLE
    , vec3 moon_position
    , float moon_radius
#endif
    , float start_i
#endif
    , vec3 earth_center, vec3 start_position, vec3 view_direction, vec3 sun_direction,
    float marching_distance, float steps, out vec3 transmittance
#ifdef MULTISCATTERING_COMPUTE_PROGRAM
    , out vec3 L_f
#endif
) {
    float r = length(start_position - earth_center);
    vec3 up_direction = normalize(start_position - earth_center);
    float mu = dot(view_direction, up_direction);
    float cos_sun_view = dot(view_direction, sun_direction);

    const float SAMPLE_COUNT = steps;
    float dx = marching_distance / SAMPLE_COUNT;

    transmittance = vec3(1.0);
    vec3 luminance = vec3(0.0);
#ifdef MULTISCATTERING_COMPUTE_PROGRAM
    L_f = vec3(0.0);
    float start_i = 0.5;
#endif
    float rayleigh_phase = RayleighPhaseFunction(cos_sun_view);
    float mie_phase = MiePhaseFunction(mie_phase_g, cos_sun_view);
    for (float i = start_i; i < SAMPLE_COUNT; ++i) {
        float d_i = i * dx;
        // Distance between the current sample point and the planet center.
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        vec3 position_i = start_position + view_direction * d_i;
        float altitude_i = r_i - bottom_radius;

        vec3 rayleigh_scattering_i;
        vec3 mie_scattering_i;
        GetScattering(altitude_i, rayleigh_scattering_i, mie_scattering_i);
        vec3 scattering_i = rayleigh_scattering_i + mie_scattering_i;
        vec3 scattering_with_phase_i = rayleigh_scattering_i * rayleigh_phase + mie_scattering_i * mie_phase;

        vec3 extinction_i = GetExtinction(altitude_i);
        vec3 transmittance_i = exp(-extinction_i * dx);
        vec3 up_direction_i = normalize(position_i - earth_center);
        float mu_s_i = dot(sun_direction, up_direction_i);
        vec3 luminance_i = scattering_with_phase_i * GetSunVisibility(transmittance_texture, r_i, mu_s_i);
#ifndef MULTISCATTERING_COMPUTE_PROGRAM
#if VOLUMETRIC_LIGHT_ENABLE
        // Shadow Map是小范围阴影，不应影响多重散射的贡献
        luminance_i *= GetVisibilityFromShadowMap(shadow_map, light_view_projection, position_i);
#endif
        vec3 multiscattering_contribution = GetMultiscatteringContribution(
            multiscattering_texture, r_i, mu_s_i);
        luminance_i += multiscattering_mask * multiscattering_contribution * scattering_i;
#if MOON_SHADOW_ENABLE
        // 月亮阴影是大范围阴影，应该影响到多重散射的贡献
        luminance_i *= GetVisibilityFromMoonShadow(moon_position - position_i, moon_radius, sun_direction);
#endif
        luminance_i *= solar_illuminance;
#endif

        luminance += transmittance * (luminance_i - luminance_i * transmittance_i) / extinction_i;
#ifdef MULTISCATTERING_COMPUTE_PROGRAM
        L_f += transmittance * (scattering_i - scattering_i * transmittance_i) / extinction_i;
#endif
        transmittance *= transmittance_i;
    }
    return luminance;
}

vec3 ComputeGroundLuminance(sampler2D transmittance_texture, vec3 earth_center, vec3 position, vec3 sun_direction
#ifndef MULTISCATTERING_COMPUTE_PROGRAM
    , vec3 ground_albedo
#endif
) {
    vec3 up_direction = normalize(position - earth_center);
    float mu_s = dot(sun_direction, up_direction);
    vec3 solar_illuminance_at_ground = GetSunVisibility(transmittance_texture, bottom_radius, mu_s);
#ifndef MULTISCATTERING_COMPUTE_PROGRAM
    solar_illuminance_at_ground *= solar_illuminance;
#endif
    vec3 normal = normalize(position - earth_center);
    return solar_illuminance_at_ground * max(dot(normal, sun_direction), 0) * ground_albedo * INV_PI;
}


#ifdef TRANSMITTANCE_COMPUTE_PROGRAM

vec3 ComputeTransmittanceToTopAtmosphereBoundary(float r, float mu) {
    const float SAMPLE_COUNT = transmittance_steps;

    float dx = DistanceToTopAtmosphereBoundary(r, mu) / SAMPLE_COUNT;

    vec3 optical_length = vec3(0.0);
    for (float i = 0.5; i < SAMPLE_COUNT; ++i) {
        float d_i = i * dx;
        // Distance between the current sample point and the planet center.
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);

        float altitude_i = r_i - bottom_radius;

        optical_length += GetExtinction(altitude_i) * dx;
    }
    return exp(-optical_length);
}

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
layout(binding = 0, rgba32f) uniform image2D transmittance_image;

void main() {
    float r, mu;
    GetRMuFromTransmittanceTextureIndex(ivec2(gl_GlobalInvocationID.xy), imageSize(transmittance_image), r, mu);
    vec3 transmittance = ComputeTransmittanceToTopAtmosphereBoundary(r, mu);
    imageStore(transmittance_image, ivec2(gl_GlobalInvocationID.xy), vec4(transmittance, 1.0));
}

#endif


#ifdef MULTISCATTERING_COMPUTE_PROGRAM

vec3 GetDirectionFromLocalIndex(int index) {
    float unit_theta = (0.5 + float(index / 8)) / 8.0;
    float unit_phi = (0.5 + float(index % 8)) / 8.0;
    float theta = PI * unit_theta;
    float phi = 2 * PI * unit_phi;
    float cos_theta = cos(theta);
    float sin_theta = sin(theta);
    float cos_phi = cos(phi);
    float sin_phi = sin(phi);
    return vec3(cos_phi * sin_theta, cos_theta, sin_phi * sin_theta);
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 64) in;
layout(binding = 0) uniform sampler2D transmittance_texture;
layout(binding = 0, rgba32f) uniform image2D multiscattering_image;

shared vec3 L_2nd_order_shared[64];
shared vec3 f_ms_shared[64];

void main() {
    float altitude, mu_s;
    GetAltitudeMuSFromMultiscatteringTextureIndex(ivec2(gl_GlobalInvocationID.xy), 
        imageSize(multiscattering_image), altitude, mu_s);
    vec3 earth_center = vec3(0, -bottom_radius, 0);
    vec3 start_position = vec3(0, altitude, 0);
    vec3 sun_direction = vec3(0, mu_s, sqrt(1 - mu_s * mu_s));
    int local_index = int(gl_GlobalInvocationID.z);
    vec3 view_direction = GetDirectionFromLocalIndex(local_index);
    
    float r = altitude + bottom_radius;
    float mu = view_direction.y;
    bool intersect_bottom = RayIntersectsGround(r, mu);
    float marching_distance = intersect_bottom ?
        DistanceToBottomAtmosphereBoundary(r, mu) :
        DistanceToTopAtmosphereBoundary(r, mu);

    vec3 transmittance;
    vec3 L_f;
    vec3 luminance = ComputeScatteredLuminance(transmittance_texture, earth_center, start_position,
        view_direction, sun_direction, marching_distance, multiscattering_steps, transmittance, L_f);

    if (intersect_bottom) {
        vec3 ground_position = start_position + view_direction * marching_distance;
        luminance += transmittance * ComputeGroundLuminance(
            transmittance_texture, earth_center, ground_position, sun_direction);
    }
    
    L_2nd_order_shared[local_index] = luminance;
    f_ms_shared[local_index] = L_f;

    barrier();
    if (local_index < 32) {
        L_2nd_order_shared[local_index] += L_2nd_order_shared[local_index + 32];
        f_ms_shared[local_index] += f_ms_shared[local_index + 32];
    }
    barrier();
    if (local_index < 16) {
        L_2nd_order_shared[local_index] += L_2nd_order_shared[local_index + 16];
        f_ms_shared[local_index] += f_ms_shared[local_index + 16];
    }
    barrier();
    if (local_index < 8) {
        L_2nd_order_shared[local_index] += L_2nd_order_shared[local_index + 8];
        f_ms_shared[local_index] += f_ms_shared[local_index + 8];
    }
    barrier();
    if (local_index < 4) {
        L_2nd_order_shared[local_index] += L_2nd_order_shared[local_index + 4];
        f_ms_shared[local_index] += f_ms_shared[local_index + 4];
    }
    barrier();
    if (local_index < 2) {
        L_2nd_order_shared[local_index] += L_2nd_order_shared[local_index + 2];
        f_ms_shared[local_index] += f_ms_shared[local_index + 2];
    }
    barrier();
    if (local_index < 1) {
        L_2nd_order_shared[local_index] += L_2nd_order_shared[local_index + 1];
        f_ms_shared[local_index] += f_ms_shared[local_index + 1];
    }
    barrier();
    if (local_index > 0)
        return;

    // 因为都是各向同性散射，直接取平均即可
    vec3 L_2nd_order = L_2nd_order_shared[0] / 64;
    vec3 f_ms = f_ms_shared[0] / 64;

    vec3 F_ms = 1 / (1 - f_ms);
    vec3 multiscattering_contribution = L_2nd_order * F_ms;

    imageStore(multiscattering_image, ivec2(gl_GlobalInvocationID.xy),
        vec4(multiscattering_contribution, 1.0));
}

#endif

#endif
