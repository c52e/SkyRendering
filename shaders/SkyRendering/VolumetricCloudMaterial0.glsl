layout(binding = MATERIAL_TEXTURE_UNIT_BEGIN + 0) uniform sampler2D cloud_map;
layout(binding = MATERIAL_TEXTURE_UNIT_BEGIN + 1) uniform sampler3D detail_texture;
layout(binding = MATERIAL_TEXTURE_UNIT_BEGIN + 2) uniform sampler2D displacement_texture;

struct SampleInfo {
    vec2 bias;
    float frequency;
    float k_lod;
};

layout(std140, binding = 3) uniform VolumetricCloudMaterialBufferData {
	SampleInfo uCloudMapSampleInfo;
	SampleInfo uDetailSampleInfo;
	SampleInfo uDisplacementSampleInfo;
	vec2 uDetailParam;
	float uLodBias;
	float uDensity;
	vec3 padding0;
	float uDisplacementScale;
};

float CalHeightMask(float cloud_type, float height01) {
    float height_in_type = clamp(height01 / cloud_type, 0, 1);
    return clamp(height_in_type * (height_in_type - 1.0) * -4.0, 0, 1);
}

float Remap01(float x, float x0, float x1) {
    return clamp((x - x0) / (x1 - x0), 0, 1);
}

vec4 GetUVWLod(vec3 pos, SampleInfo sample_info) {
    vec3 uvw = pos * sample_info.frequency + vec3(sample_info.bias, 0.0);
    float lod = log2(sample_info.k_lod * distance(pos, uCameraPos)) + uLodBias;
    return vec4(uvw, lod);
}

float SampleSigmaT(vec3 pos, float height01) {
    vec4 uvwlod = GetUVWLod(pos, uCloudMapSampleInfo);
    vec2 cloud_type = textureLod(cloud_map, uvwlod.xy, uvwlod.a).rg;
    
    vec3 displace_vector = vec3(0);
    uvwlod = GetUVWLod(pos, uDisplacementSampleInfo);
    displace_vector.xy += textureLod(displacement_texture, uvwlod.xy, uvwlod.a).rg;
    displace_vector.xz += textureLod(displacement_texture, uvwlod.xz, uvwlod.a).ba;

    pos += uDisplacementScale * displace_vector;
    uvwlod = GetUVWLod(pos, uDetailSampleInfo);
    float detail = textureLod(detail_texture, uvwlod.xyz, uvwlod.a).r;
    detail = detail * uDetailParam.x + uDetailParam.y;
    return Remap01(cloud_type.r * CalHeightMask(cloud_type.g, height01), detail, 1.0) * height01 * uDensity;
}