#ifndef __VOLUMETRIC_CLOUD_COMMON_GLSL
#define __VOLUMETRIC_CLOUD_COMMON_GLSL

layout(std140, binding = 1) uniform VolumetricCloudCommonBufferData{
	mat4 uInvMVP;
	mat4 uPreMVP;
	vec3 uCameraPos;
	uint uBaseShadingIndex;
	vec2 uFrameDelta;
	vec2 uLinearDepthParam; // x = 1 / zNear; y = (zFar - zNear) / (zFar * zNear)
};

const float kMinTransmittance = 0.01;

float DepthToLinearDepth(float depth) {
	return 1.0 / (uLinearDepthParam.x - uLinearDepthParam.y * depth);
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

#endif