layout(std140, binding = 3) uniform VolumetricCloudMaterialBufferData {
	vec3 padding;
	float uDensity;
};

float SampleSigmaT(vec3 pos, float height01) {
    return uDensity;
}