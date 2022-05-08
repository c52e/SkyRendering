#pragma once

#include <glm/glm.hpp>

#include "IVolumetricCloudMaterial.h"
#include "Samplers.h"
#include "GLReloadableProgram.h"

class VolumetricCloudDefaultMaterial : public IVolumetricCloudMaterial {
public:
    VolumetricCloudDefaultMaterial();

    std::string ShaderPath() override;

    void Setup(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) override;

    void DrawGUI() override;

private:
    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(cloud_map_.repeat_size)
        FIELD_DECLARE(detail_texture_.repeat_size)
        FIELD_DECLARE(displacement_texture_.repeat_size)
        FIELD_DECLARE(density_)
        FIELD_DECLARE(lod_bias_)
        FIELD_DECLARE(wind_speed_)
        FIELD_DECLARE(detail_wind_magnify_)
        FIELD_DECLARE(detail_param_)
        FIELD_DECLARE(minfilter2d_)
        FIELD_DECLARE(minfilter3d_)
        FIELD_DECLARE(minfilter_displacement_)
        FIELD_DECLARE(displacement_scale_)
    FIELD_DECLARATION_END()

    struct BufferData;

    GLBuffer buffer_;

    float lod_bias_ = 2.75f;
    float density_ = 15.0f;
    float wind_speed_ = 0.05f;
    float detail_wind_magnify_ = 1.0f;
    glm::vec2 detail_param_{ 0.4f, 0.0f };
    float displacement_scale_ = 1.0f;
    TextureWithInfo cloud_map_;
    TextureWithInfo detail_texture_;
    TextureWithInfo displacement_texture_;

    Samplers::MipmapMin minfilter2d_ = Samplers::MipmapMin::NEAREST_MIPMAP_NEAREST;
    Samplers::MipmapMin minfilter3d_ = Samplers::MipmapMin::NEAREST_MIPMAP_NEAREST;
    Samplers::MipmapMin minfilter_displacement_ = Samplers::MipmapMin::NEAREST_MIPMAP_NEAREST;

    glm::dvec2 detail_offset_from_first_{ 0, 0 };

    GLReloadableComputeProgram cloud_map_gen_program_;
    GLReloadableComputeProgram detail_gen_program_;
    GLReloadableComputeProgram displacement_gen_program_;
};
