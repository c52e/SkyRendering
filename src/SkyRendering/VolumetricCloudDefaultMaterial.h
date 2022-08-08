#pragma once

#include <glm/glm.hpp>

#include "IVolumetricCloudMaterial.h"
#include "Samplers.h"
#include "GLReloadableProgram.h"

struct TextureWithInfo {
    GLuint id() { return tex.id(); }

    GLTexture tex;
    float repeat_size = 20.0f;
    int x = 1;
    int y = 1;
    int z = 1;
    int channel = 1;
};

template<class BufferType>
class DynamicTexture {
public:
    GLReloadableComputeProgram program;
    TextureWithInfo texture;
    BufferType buffer;

    DynamicTexture() {
        gl_buffer_.Create();
        glNamedBufferStorage(gl_buffer_.id(), sizeof(buffer), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    void GenerateIfParameterChanged() {
        if (is_first_update_ || memcmp(&buffer, &pre_buffer_, sizeof(buffer)) != 0) {
            Generate();
            pre_buffer_ = buffer;
        }
    }

    void Generate() {
        glNamedBufferSubData(gl_buffer_.id(), 0, sizeof(buffer), &buffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, gl_buffer_.id());

        GLBindImageTextures({ texture.id() });
        glUseProgram(program.id());
        program.Dispatch(glm::ivec3(texture.x, texture.y, texture.z));
        // https://stackoverflow.com/questions/24693861/which-memory-barrier-does-glgeneratemipmap-require
        glGenerateTextureMipmap(texture.id());
        is_first_update_ = false;
    }

private:
    GLBuffer gl_buffer_;
    BufferType pre_buffer_;
    bool is_first_update_ = true;
};

struct NoiseCreateInfo {
    int seed;
    int base_frequency;
    float remap_min;
    float remap_max;

    void DrawGUI();
};

struct CloudMapBuffer {
    NoiseCreateInfo uDensity{ 0, 3, 0.35f, 0.75f };
    NoiseCreateInfo uHeight{ 0, 5, 0.8f, 0.4f };

    void DrawGUI();
};

struct DetailBuffer {
    NoiseCreateInfo uPerlin{ 0, 7, 0.23f, 1.0f };
    NoiseCreateInfo uWorley{ 0, 11, 1.0f, 0.0f };

    void DrawGUI();
};

struct DisplacementBuffer {
    NoiseCreateInfo uPerlin{ 0, 6, 0.25f, 0.75f };

    void DrawGUI();
};

class VolumetricCloudDefaultMaterialCommon : public ISerializable {
public:
    VolumetricCloudDefaultMaterialCommon();

    void Update(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta);

    void Bind();

    float GetSigmaTMax();

    void DrawGUI();

private:
#define DECLARE_NOISE(name) \
    FIELD_DECLARE(name.seed) \
    FIELD_DECLARE(name.base_frequency) \
    FIELD_DECLARE(name.remap_min) \
    FIELD_DECLARE(name.remap_max)

    FIELD_DECLARATION_BEGIN(ISerializable)
        DECLARE_NOISE(cloud_map_.buffer.uDensity)
        DECLARE_NOISE(cloud_map_.buffer.uHeight)
        DECLARE_NOISE(detail_.buffer.uPerlin)
        DECLARE_NOISE(detail_.buffer.uWorley)
        DECLARE_NOISE(displacement_.buffer.uPerlin)
        FIELD_DECLARE(cloud_map_.texture.repeat_size)
        FIELD_DECLARE(detail_.texture.repeat_size)
        FIELD_DECLARE(displacement_.texture.repeat_size)
        FIELD_DECLARE(density_)
        FIELD_DECLARE(lod_bias_)
        FIELD_DECLARE(wind_speed_)
        FIELD_DECLARE(detail_wind_magnify_)
        FIELD_DECLARE(minfilter2d_)
        FIELD_DECLARE(minfilter3d_)
        FIELD_DECLARE(minfilter_displacement_)
    FIELD_DECLARATION_END()

#undef DECLARE_NOISE
    GLBuffer buffer_;

    float lod_bias_ = 2.75f;
    float density_ = 15.0f;
    float wind_speed_ = 0.05f;
    float detail_wind_magnify_ = 1.0f;

    Samplers::MipmapMin minfilter2d_ = Samplers::MipmapMin::NEAREST_MIPMAP_NEAREST;
    Samplers::MipmapMin minfilter3d_ = Samplers::MipmapMin::NEAREST_MIPMAP_NEAREST;
    Samplers::MipmapMin minfilter_displacement_ = Samplers::MipmapMin::NEAREST_MIPMAP_NEAREST;

    glm::dvec2 detail_offset_from_first_{ 0, 0 };

    DynamicTexture<CloudMapBuffer> cloud_map_;
    DynamicTexture<DetailBuffer> detail_;
    DynamicTexture<DisplacementBuffer> displacement_;
};

class VolumetricCloudDefaultMaterial0 : public IVolumetricCloudMaterial {
public:
    VolumetricCloudDefaultMaterial0();

    std::string ShaderPath() override;

    void Update(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) override;

    void Bind() override;

    float GetSigmaTMax() override;

    void DrawGUI() override;

private:
    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(materail_common_)
        FIELD_DECLARE(detail_param_)
        FIELD_DECLARE(displacement_scale_)
    FIELD_DECLARATION_END()

    GLBuffer buffer_;

    glm::vec2 detail_param_{ 0.4f, 0.0f };
    float displacement_scale_ = 1.0f;

    VolumetricCloudDefaultMaterialCommon materail_common_;
};

class VolumetricCloudDefaultMaterial1 : public IVolumetricCloudMaterial {
public:
    VolumetricCloudDefaultMaterial1();

    std::string ShaderPath() override;

    void Update(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) override;

    void Bind() override;

    float GetSigmaTMax() override;

    void DrawGUI() override;

private:
    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(materail_common_)
        FIELD_DECLARE(base_density_threshold_)
        FIELD_DECLARE(base_height_hardness_)
        FIELD_DECLARE(base_edge_hardness_)
        FIELD_DECLARE(detail_base_)
        FIELD_DECLARE(detail_scale_)
        FIELD_DECLARE(height_cut_)
        FIELD_DECLARE(edge_cut_)
    FIELD_DECLARATION_END()

    GLBuffer buffer_;

    float detail_base_ = 0.67f;
    float detail_scale_ = 1.86f;
    float base_density_threshold_ = 0.4f;
    float base_height_hardness_ = 6.0f;
    float base_edge_hardness_ = 6.0f;
    float height_cut_ = 0.9f;
    float edge_cut_ = 0.8f;

    VolumetricCloudDefaultMaterialCommon materail_common_;
};
