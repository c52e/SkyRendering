#include "VolumetricCloudDefaultMaterialCommon.glsl"

layout(std140, binding = 4) uniform VolumetricCloudMaterialBufferData{
    float uBaseDensityThreshold;
    float uBaseHeightHardness;
    float uBaseEdgeHardness;
    float uDetailBase;
    float uDetailScale;
    float uHeightCut;
    float uEdgeCur;
    float padding1;
};

float SampleSigmaT(vec3 pos, float height01) {
    vec4 uvwlod = GetUVWLod(pos, uCloudMapSampleInfo);
    vec2 cloud_type = textureLod(cloud_map, uvwlod.xy, uvwlod.a).rg;
    
    float density = clamp((cloud_type.r - uBaseDensityThreshold) * uBaseEdgeHardness, 0, 1);
    density *= clamp((1 - height01) * uBaseHeightHardness, 0, 1);
    if (density == 0)
        return 0;

    uvwlod = GetUVWLod(pos, uDetailSampleInfo);
    float detail = textureLod(detail_texture, uvwlod.xyz, uvwlod.a).r;
    detail = (detail + uDetailBase) * uDetailScale;
    detail *= max(clamp(height01 - uHeightCut, 0, 1), clamp(uEdgeCur - cloud_type.r, 0, 1));
    density = clamp(density - detail, 0, 1) * uDensity * height01;
    return density;
}