layout(binding = MATERIAL_TEXTURE_UNIT_BEGIN + 0) uniform sampler3D voxel;

layout(std140, binding = 3) uniform VolumetricCloudMaterialBufferData{
	vec2 uSampleFrequency;
	float uLodBias;
	float uDensity;
	vec2 uSampleBias;
	float uSampleLodK;
	float voxel_material_padding;
};

float SampleSigmaT(vec3 pos, float height01) {
    vec2 uv = pos.xy * uSampleFrequency + uSampleBias;
    float lod = log2(uSampleLodK * distance(pos, uCameraPos)) + uLodBias;
    float density = textureLod(voxel, vec3(uv, height01), lod).r;
	return density * uDensity;
}