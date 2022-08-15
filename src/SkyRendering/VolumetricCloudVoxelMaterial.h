#pragma once

#include "IVolumetricCloudMaterial.h"

class VolumetricCloudVoxelMaterial : public IVolumetricCloudMaterial {
public:
    VolumetricCloudVoxelMaterial();

    std::string ShaderPath() override;

    void Update(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) override;

    void Bind() override;

    float GetSigmaTMax() override;

    void DrawGUI() override;

private:
    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(lod_bias_)
        FIELD_DECLARE(density_)
        FIELD_DECLARE(base_)
        FIELD_DECLARE(width_)
    FIELD_DECLARATION_END()

    struct BufferData;

    GLBuffer buffer_;
    GLTexture voxel_;
    GLSampler sampler_;
    glm::ivec3 voxel_dim_{ 1,1,1 };

    float lod_bias_ = 2.75f;
    float density_ = 20.0f;
    glm::vec2 base_{ 0.0f, 0.0f };
    glm::vec2 width_{ 2.0f, 2.0f };
};
