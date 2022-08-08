#ifndef _VOLUMETRIC_CLOUD_COMMON_GLSL
#define _VOLUMETRIC_CLOUD_COMMON_GLSL

#include "AtmosphereInterface.glsl"

layout(std140, binding = 1) uniform VolumetricCloudCommonBufferData{
	mat4 uInvMVP;
	mat4 uReprojectMat;

	mat4 uLightVP;
	mat4 uInvLightVP;
	mat4 uShadowMapReprojectMat;

	vec3 uCameraPos;
	uint uBaseShadingIndex;

	vec2 uLinearDepthParam; // x = 1 / zNear; y = (zFar - zNear) / (zFar * zNear)
	float uBottomAltitude;
	float uTopAltitude;

	vec3 uSunDirection;
	float uFrameID;

	float uInvShadowFroxelMaxDistance;
	float uAerialPerspectiveLutMaxDistance;
	float uShadowFroxelMaxDistance;
	float uEarthRadius;
};

const float kMinTransmittance = 0.01;

float DepthToLinearDepth(float depth) {
	return 1.0 / (uLinearDepthParam.x - uLinearDepthParam.y * depth);
}

float CalHeight01(vec3 pos) {
	float altitude = length(vec3(pos.xy, pos.z + uEarthRadius)) - uEarthRadius;
	return clamp((altitude - uBottomAltitude) / (uTopAltitude - uBottomAltitude), 0, 1);
}

// Same order as textureGather
ivec2 IndexToOffset(uint index) {
#if 0
	const ivec2 offsets[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
	return offsets[index];
#else
	return ivec2(
		((index + 1) >> 1) & 1,
		((index + 2) >> 1) & 1
	);
#endif
}

vec4 texelFetchClamp(sampler2D s, ivec2 p, int lod) {
	return texelFetch(s, clamp(p, ivec2(0), textureSize(s, lod) - 1), lod);
}

float HenyeyGreenstein(float cos_theta, float g) {
	float a = 1.0 - g * g;
	float b = 1.0 + g * g - 2.0 * g * cos_theta;
	b *= sqrt(b);
	return (0.25 * INV_PI) * a / b;
}

vec3 GetSunVisibility(sampler2D transmittance_texture, vec3 pos) {
	vec3 up_dir = vec3(pos.xy, pos.z + uEarthRadius);
	float r = length(up_dir);
	up_dir /= r;
	float mu_s = dot(uSunDirection, up_dir);
	return GetSunVisibility(transmittance_texture, r, mu_s);
}

vec3 GetAerialPerspective(sampler3D aerial_perspective_luminance_texture,
		sampler3D aerial_perspective_transmittance_texture,
		vec2 uv, float t, float r, float mu, out vec3 transmittance) {
	if (r > top_radius) {
		float near_distance;
		if (FromSpaceIntersectTopAtmosphereBoundary(r, mu, near_distance)) {
			t -= near_distance;
		}
		else {
			t = 0;
		}
	}
	vec3 uvw = GetAerialPerspectiveTextureUvwFromTexCoordDistance(
		uv, t, uAerialPerspectiveLutMaxDistance, textureSize(aerial_perspective_luminance_texture, 0));
	transmittance = texture(aerial_perspective_transmittance_texture, uvw).rgb;
	vec3 luminance = texture(aerial_perspective_luminance_texture, uvw).rgb;
	return luminance;
}

#endif