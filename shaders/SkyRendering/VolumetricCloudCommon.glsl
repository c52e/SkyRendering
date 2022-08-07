#ifndef _VOLUMETRIC_CLOUD_COMMON_GLSL
#define _VOLUMETRIC_CLOUD_COMMON_GLSL

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

	vec2 _cloud_common_padding;
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

#endif