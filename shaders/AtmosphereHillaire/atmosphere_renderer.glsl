
layout(binding = 0) uniform sampler2D transmittance_texture;
layout(binding = 1) uniform sampler2D multiscattering_texture;
layout(binding = 2) uniform sampler2D depth_stencil_texture;
layout(binding = 3) uniform sampler2D normal_texture;
layout(binding = 4) uniform sampler2D albedo_texture;
layout(binding = 5) uniform sampler2DShadow shadow_map_texture;
layout(binding = 6) uniform sampler2D blue_noise_texture;
layout(binding = 7) uniform sampler2D star_texture;
layout(binding = 8) uniform sampler2D earth_texture;
layout(binding = 9) uniform sampler2D sky_view_luminance_texture;
layout(binding = 10) uniform sampler2D sky_view_transmittance_texture;
layout(binding = 11) uniform sampler3D aerial_perspective_luminance_texture;
layout(binding = 12) uniform sampler3D aerial_perspective_transmittance_texture;

layout(std140, binding = 1) uniform AtmosphereRenderBufferData{
    vec3 sun_direction;
    float exposure;

    vec3 camera_position;
    float star_luminance_scale;

    mat4 inv_view_projection;
    mat4 light_view_projection;

    vec3 up_direction;
    float camera_earth_center_distance;
    vec3 right_direction;
    float raymarching_steps;
    vec3 front_direction;
    float sky_view_lut_steps;
    vec2 padding10;
    float aerial_perspective_lut_steps;
    float aerial_perspective_lut_max_distance;
    vec3 moon_position;
    float moon_radius;
};

vec3 GetWorldPositionFromNDCDepth(mat4 inv_view_projection, vec2 ndc, float depth) {
    vec4 xyzw = inv_view_projection * vec4(ndc, depth * 2.0 - 1.0, 1.0);
    return xyzw.xyz / xyzw.w;
}

bool FromSpaceIntersectTopAtmosphereBoundary(float r, float mu, out float near_distance) {
    float discriminant = r * r * (mu * mu - 1.0) + top_radius * top_radius;
    if (mu < 0.0 && discriminant >= 0.0) {
        near_distance = ClampDistance(-r * mu - SafeSqrt(discriminant));
        return true;
    }
    return false;
}

vec3 ComputeRaymarchingStartPositionAndChangeDistance(vec3 view_direction, inout float marching_distance) {
    vec3 start_position = camera_position;
    bool in_space = camera_earth_center_distance > top_radius;
    if (in_space) {
        float r = camera_earth_center_distance;
        float mu = dot(view_direction, up_direction);
        float near_distance;
        if (FromSpaceIntersectTopAtmosphereBoundary(r, mu, near_distance)) {
            start_position += near_distance * view_direction;
            marching_distance -= near_distance;
        }
        else {
            marching_distance = 0;
        }
    }
    return start_position;
}

float GetHorizonDownAngleFromR(float r) {
    float tangent_point_distance = sqrt(r * r - bottom_radius * bottom_radius);
    float cos_horizon_down = tangent_point_distance / r;
    return acos(cos_horizon_down);
}

// 视线在太阳，地心，相机形成的平面上时，Lon=0
void GetCosLatLonFromSkyViewTextureIndex(ivec2 index, float r, out float cos_lat, out float cos_lon) {
    vec2 uv = vec2(index) / vec2(SKY_VIEW_LUT_SIZE - 1);
    float x_cos_lon = uv.x;
    float x_cos_lat = uv.y;

    float horizon_down_angle = GetHorizonDownAngleFromR(r);
    float horizon_up_angle = PI - horizon_down_angle;

    float lat;
    if (x_cos_lat < 0.5f) {
        float coord = 1.0 - 2.0 * x_cos_lat;
        coord = 1.0 - coord * coord;
        lat = horizon_up_angle * coord;
    }
    else {
        float coord = x_cos_lat * 2.0 - 1.0;
        coord *= coord;
        lat = horizon_up_angle + horizon_down_angle * coord;
    }
    cos_lat = cos(lat);
    cos_lon = -(x_cos_lon * x_cos_lon * 2.0 - 1.0);
}

vec3 GetViewDirectionFromCosLatLon(float cos_lat, float cos_lon) {
    float sin_lat = sqrt(1 - cos_lat * cos_lat);
    float sin_lon = sqrt(1 - cos_lon * cos_lon);
    vec3 view_direction = up_direction * cos_lat +
        front_direction * sin_lat * cos_lon +
        right_direction * sin_lat * sin_lon;
    return view_direction;
}

vec2 GetSkyViewTextureUvFromCosLatLon(float r, float cos_lat, float cos_lon) {
    float horizon_down_angle = GetHorizonDownAngleFromR(r);
    float horizon_up_angle = PI - horizon_down_angle;

    float lat = acos(cos_lat);
    float x_cos_lat;
    if (lat < horizon_up_angle) {
        float coord = lat / horizon_up_angle;
        coord = sqrt(1 - coord);
        x_cos_lat = 0.5 - 0.5 * coord;
    }
    else {
        float coord = (lat - horizon_up_angle) / horizon_down_angle;
        coord = sqrt(coord);
        x_cos_lat = coord * 0.5 + 0.5;
    }

    float x_cos_lon = sqrt(0.5 - 0.5 * cos_lon);
    return GetTextureCoordFromUnitRange(vec2(x_cos_lon, x_cos_lat), SKY_VIEW_LUT_SIZE);
}

void GetCosLatLonFromViewDirection(vec3 view_direction, out float cos_lat, out float cos_lon) {
    cos_lat = dot(up_direction, view_direction);
    vec3 lon_direction = view_direction - up_direction * cos_lat;
    float lon_direction_length = length(lon_direction);
    if (lon_direction_length == 0) {
        cos_lon = 1;
    }
    else {
        lon_direction /= lon_direction_length;
        cos_lon = dot(lon_direction, front_direction);
    }
}

#ifdef SKY_VIEW_COMPUTE_PROGRAM

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
layout(binding = 0, rgba32f) uniform image2D luminance_image;
layout(binding = 1, rgba32f) uniform image2D transmittance_image;

void main() {
    float r = camera_earth_center_distance;
    float cos_lat, cos_lon;
    GetCosLatLonFromSkyViewTextureIndex(ivec2(gl_GlobalInvocationID.xy), r, cos_lat, cos_lon);
    vec3 view_direction = GetViewDirectionFromCosLatLon(cos_lat, cos_lon);

    float mu = cos_lat;
    bool intersect_bottom = RayIntersectsGround(r, mu);
    float marching_distance = intersect_bottom ?
        DistanceToBottomAtmosphereBoundary(r, mu) :
        DistanceToTopAtmosphereBoundary(r, mu);

    vec3 start_position = ComputeRaymarchingStartPositionAndChangeDistance(view_direction, marching_distance);
    vec3 transmittance = vec3(1);
    vec3 luminance = vec3(0);
    if (marching_distance > 0) {
#if DITHER_SAMPLE_POINT_ENABLE
        float start_i = texelFetch(blue_noise_texture, ivec2(gl_GlobalInvocationID.xy) & 0x3f, 0).x;
#else
        float start_i = 0.5;
#endif
        luminance = ComputeScatteredLuminance(transmittance_texture, multiscattering_texture,
#if VOLUMETRIC_LIGHT_ENABLE
            shadow_map_texture, light_view_projection, 
#endif
#if MOON_SHADOW_ENABLE
            moon_position, moon_radius,
#endif
            start_i, start_position, view_direction,
            sun_direction, marching_distance, sky_view_lut_steps, transmittance);
    }
    imageStore(luminance_image, ivec2(gl_GlobalInvocationID.xy), vec4(luminance, 0.0));
    imageStore(transmittance_image, ivec2(gl_GlobalInvocationID.xy), vec4(transmittance, 0.0));
}

#endif


vec3 GetPositionFromAerialPerspectiveTextureIndex(ivec3 index,
        vec3 camera_position, mat4 inv_view_projection, out vec3 view_direction) {
    vec3 uvw = vec3(index) / vec3(AERIAL_PERSPECTIVE_LUT_SIZE - 1);
    vec3 position = GetWorldPositionFromNDCDepth(inv_view_projection, uvw.xy * 2.0 - 1.0, 0);
    view_direction = normalize(position - camera_position);
    float distance_from_camera = uvw.z * uvw.z * aerial_perspective_lut_max_distance;
    position = camera_position + view_direction * distance_from_camera;
    return position;
}

vec3 GetTextureCoordFromUnitRange(vec3 xyz, ivec3 texture_size) {
    return 0.5 / vec3(texture_size) + xyz * (1.0 - 1.0 / vec3(texture_size));
}

vec3 GetAerialPerspectiveTextureUvwFromTexCoordDistance(vec2 TexCoord, float marching_distance) {
    return GetTextureCoordFromUnitRange(vec3(TexCoord, 
        sqrt(marching_distance / aerial_perspective_lut_max_distance)), AERIAL_PERSPECTIVE_LUT_SIZE);
}

#ifdef AERIAL_PERSPECTIVE_COMPUTE_PROGRAM

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
layout(binding = 0, rgba32f) uniform image3D luminance_image;
layout(binding = 1, rgba32f) uniform image3D transmittance_image;

void main() {
    float r = camera_earth_center_distance;
    vec3 view_direction;
    vec3 fragment_position = GetPositionFromAerialPerspectiveTextureIndex(
        ivec3(gl_GlobalInvocationID.xyz), camera_position, inv_view_projection, view_direction);
    vec3 earth_to_fragment = fragment_position - earth_center;
    float fragment_r = length(earth_to_fragment);
    if (fragment_r < bottom_radius) {
        fragment_position = earth_center + earth_to_fragment / fragment_r * bottom_radius;
        view_direction = normalize(fragment_position - camera_position);
    }
    float marching_distance = distance(camera_position, fragment_position);


    vec3 start_position = ComputeRaymarchingStartPositionAndChangeDistance(view_direction, marching_distance);
    vec3 transmittance = vec3(1);
    vec3 luminance = vec3(0);
    if (marching_distance > 0) {
#if DITHER_SAMPLE_POINT_ENABLE
        float start_i = texelFetch(blue_noise_texture, ivec2(gl_GlobalInvocationID.xy) & 0x3f, 0).x;
#else
        float start_i = 0.5;
#endif
        luminance = ComputeScatteredLuminance(transmittance_texture, multiscattering_texture,
#if VOLUMETRIC_LIGHT_ENABLE
            shadow_map_texture, light_view_projection,
#endif
#if MOON_SHADOW_ENABLE
            moon_position, moon_radius,
#endif
            start_i, start_position, view_direction,
            sun_direction, marching_distance, aerial_perspective_lut_steps, transmittance);
    }
    imageStore(luminance_image, ivec3(gl_GlobalInvocationID.xyz), vec4(luminance, 0.0));
    imageStore(transmittance_image, ivec3(gl_GlobalInvocationID.xyz), vec4(transmittance, 0.0));
}

#endif


#ifdef ATMOSPHERE_RENDER_FRAGMENT_SHADER

in vec2 vTexCoord;
in vec2 vNDC;
layout(location = 0) out vec4 FragColor;

vec3 ComputeObjectLuminance(sampler2D transmittance_texture, vec3 position,
        vec3 normal, vec3 albedo, vec3 sun_direction) {
    float r = length(position - earth_center);
    vec3 object_up_direction = normalize(position - earth_center);
    float mu_s = dot(sun_direction, object_up_direction);

    vec3 sun_visibility;
    if (r > top_radius) {
        float near_distance;
        if (FromSpaceIntersectTopAtmosphereBoundary(r, mu_s, near_distance)) {
            position += near_distance * sun_direction;
            r = length(position - earth_center);
            object_up_direction = normalize(position - earth_center);
            mu_s = dot(sun_direction, object_up_direction);
            sun_visibility = GetSunVisibility(transmittance_texture, r, mu_s);
        }
        else {
            sun_visibility = vec3(1);
        }
    }
    else {
        sun_visibility = GetSunVisibility(transmittance_texture, r, mu_s);
    }

    vec3 solar_illuminance_at_object = solar_illuminance * sun_visibility;
    return solar_illuminance_at_object * max(dot(normal, sun_direction), 0) * albedo / PI;
}

vec3 GetStarLuminance(sampler2D star_texture, vec3 view_direction) {
    float theta = acos(view_direction.y);
    float phi = atan(view_direction.x, view_direction.z);
    vec2 coord = vec2((phi + PI) / (2 * PI), theta / PI);
    return star_luminance_scale * pow(texture(star_texture, coord).xyz, vec3(2.2));
}

vec3 GetEarthAlbedo(sampler2D earth_texture, vec3 ground_position) {
    vec3 direction = normalize(ground_position - earth_center);
    float theta = acos(direction.y);
    float phi = atan(direction.x, direction.z);
    vec2 coord = vec2((phi + PI) / (2 * PI), theta / PI);
    return pow(texture(earth_texture, coord).xyz, vec3(2.2));
}

// 近似的环境光，使得晚上不至于地面全黑
vec3 ComputeLuminanceLightedByStars(vec3 albedo) {
    return 0.3 * star_luminance_scale * albedo / PI;
}

vec3 CEToneMapping(vec3 luminance, float adapted_lum) {
    return 1 - exp(-adapted_lum * luminance);
}

// https://zhuanlan.zhihu.com/p/21983679
vec3 ACESToneMapping(vec3 luminance, float adapted_lum) {
    const float k = 10.0 / 16.0; // Make CEToneMapping ACESToneMapping produce similar result with same adpted_lum.

    const float A = 2.51f * k * k;
    const float B = 0.03f * k;
    const float C = 2.43f * k * k;
    const float D = 0.59f * k;
    const float E = 0.14f;

    luminance *= adapted_lum;
    return (luminance * (A * luminance + B)) / (luminance * (C * luminance + D) + E);
}

void main() {
    float depth = texture(depth_stencil_texture, vTexCoord).x;
    vec3 fragment_position = GetWorldPositionFromNDCDepth(inv_view_projection, vNDC, depth);
    vec3 view_direction = normalize(fragment_position - camera_position);
    float r = camera_earth_center_distance;
    float mu = dot(view_direction, up_direction);
    bool intersect_bottom = RayIntersectsGround(r, mu);

    float marching_distance = intersect_bottom ?
        DistanceToBottomAtmosphereBoundary(r, mu) :
        DistanceToTopAtmosphereBoundary(r, mu);

    bool intersect_object = false;
    if (depth != 1.0) {
        float object_distance = length(fragment_position - camera_position);
        intersect_object = length(fragment_position - earth_center) > bottom_radius;
        if (intersect_bottom)
            intersect_object = intersect_object && object_distance < marching_distance;
        marching_distance = min(marching_distance, object_distance);
    }

    vec3 start_position = ComputeRaymarchingStartPositionAndChangeDistance(view_direction, marching_distance);
    vec3 transmittance = vec3(1);
    vec3 luminance = vec3(0);
    if (marching_distance > 0) {
#if DITHER_SAMPLE_POINT_ENABLE
        float start_i = texelFetch(blue_noise_texture, ivec2(gl_FragCoord.xy) & 0x3f, 0).x;
#else
        float start_i = 0.5;
#endif
#if USE_SKY_VIEW_LUT
        if (!intersect_object) {
            float cos_lat, cos_lon;
            GetCosLatLonFromViewDirection(view_direction, cos_lat, cos_lon);
            vec2 uv = GetSkyViewTextureUvFromCosLatLon(r, cos_lat, cos_lon);
            luminance = texture(sky_view_luminance_texture, uv).rgb;
            transmittance = texture(sky_view_transmittance_texture, uv).rgb;
        }
        else
#endif
#if USE_AERIAL_PERSPECTIVE_LUT
        if (intersect_object) {
            vec3 uvw = GetAerialPerspectiveTextureUvwFromTexCoordDistance(vTexCoord, marching_distance);
            luminance = texture(aerial_perspective_luminance_texture, uvw).rgb;
            transmittance = texture(aerial_perspective_transmittance_texture, uvw).rgb;
        }
        else
#endif
            luminance = ComputeScatteredLuminance(transmittance_texture, multiscattering_texture,
#if VOLUMETRIC_LIGHT_ENABLE
                shadow_map_texture, light_view_projection,
#endif
#if MOON_SHADOW_ENABLE
                moon_position, moon_radius,
#endif
                start_i, start_position, view_direction,
                sun_direction, marching_distance, raymarching_steps, transmittance);

    }

    if (intersect_object) {
        vec3 normal = texture(normal_texture, vTexCoord).rgb;
        vec3 albedo = texture(albedo_texture, vTexCoord).rgb;
        vec3 visibility = transmittance;
        visibility *= GetVisibilityFromShadowMap(shadow_map_texture, light_view_projection, fragment_position);
#if MOON_SHADOW_ENABLE
        vec3 moon_direction = moon_position - fragment_position;
        float moon_distance = length(moon_direction);
        moon_direction /= moon_distance;
        float moon_angular_radius = asin(clamp(moon_radius / moon_distance, -1, 1));
        visibility *= GetVisibilityFromMoonShadow(acos(clamp(dot(sun_direction, moon_direction), -1, 1)), 
            sun_angular_radius, moon_angular_radius);
#endif
        luminance += visibility * ComputeObjectLuminance(
            transmittance_texture, fragment_position, normal, albedo, sun_direction);
        luminance += transmittance * ComputeLuminanceLightedByStars(albedo);
    }
    else if (intersect_bottom) {
        vec3 ground_position = start_position + view_direction * marching_distance;
        vec3 visibility = transmittance;
        visibility *= GetVisibilityFromShadowMap(shadow_map_texture, light_view_projection, ground_position);
#if MOON_SHADOW_ENABLE
        vec3 moon_direction = moon_position - ground_position;
        float moon_distance = length(moon_direction);
        moon_direction /= moon_distance;
        float moon_angular_radius = asin(moon_radius / moon_distance);
        visibility *= GetVisibilityFromMoonShadow(acos(clamp(dot(sun_direction, moon_direction), -1, 1)),
            sun_angular_radius, moon_angular_radius);
#endif
        vec3 albedo = GetEarthAlbedo(earth_texture, ground_position);
        luminance += visibility * ComputeGroundLuminance(
            transmittance_texture, ground_position, sun_direction, albedo);
        luminance += transmittance * ComputeLuminanceLightedByStars(albedo);
    }
    else if (dot(view_direction, sun_direction) >= cos(sun_angular_radius)) {
        // https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
        // B Sun limb darkening astro-physical models
        vec3 u = vec3(1.0, 1.0, 1.0);
        vec3 a = vec3(0.397, 0.503, 0.652);

        float cos_view_sun = dot(view_direction, sun_direction);
        float sin_view_sun = sqrt(1.0 - cos_view_sun * cos_view_sun);
        
        float center_to_edge = clamp(sin_view_sun / sin(sun_angular_radius), 0.0, 1.0);
        float mu = sqrt(1.0 - center_to_edge * center_to_edge);

        vec3 factor = 1.0 - u * (1.0 - pow(vec3(mu), a));

        vec3 solar_illuminance_at_eye = solar_illuminance * transmittance;
        luminance += solar_illuminance_at_eye / (PI * sun_angular_radius * sun_angular_radius) * factor;
    } else {
        luminance += transmittance * GetStarLuminance(star_texture, view_direction);
    }
#if DITHER_COLOR_ENABLE
    float blue_noise = texelFetch(blue_noise_texture, ivec2(gl_FragCoord.xy) & 0x3f, 0).x;
#else
    float blue_noise = 0;
#endif
    FragColor = vec4(pow(TONE_MAPPING(luminance, exposure), vec3(1.0 / 2.2)) + blue_noise / 255.0, 1.0);
}

#endif
