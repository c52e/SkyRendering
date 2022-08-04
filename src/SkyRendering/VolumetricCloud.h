#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gl.hpp"
#include "GLReloadableProgram.h"
#include "PerformanceMarker.h"
#include "Earth.h"
#include "AtmosphereRenderer.h"
#include "Camera.h"
#include "Serialization.h"
#include "IVolumetricCloudMaterial.h"
#include "Samplers.h"

class VolumetricCloud : public ISerializable {
public:
    std::unique_ptr<IVolumetricCloudMaterial> material;

    float bottom_altitude_ = 2.0f;
    float thickness_ = 2.0f;
    float max_raymarch_distance_ = 30.0f;
    float max_raymarch_steps_ = 128.0f;
    float max_visible_distance_ = 120.0f;
    glm::vec3 env_color_{ 1, 1, 1 };
    float env_color_scale_ = 0.1f;
    float sun_illuminance_scale_ = 1.0f;
    float shadow_steps_ = 5.0f;
    float shadow_distance_ = 2.0f;
    float shadow_map_max_distance = 20.0f;
    float shadow_froxel_max_distance = 20.0f;
    float sun_multiscattering_sigma_scale = 0.3f;
    float env_multiscattering_sigma_scale = 0.5f;

    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(material)
        FIELD_DECLARE(bottom_altitude_)
        FIELD_DECLARE(thickness_)
        FIELD_DECLARE(max_raymarch_distance_)
        FIELD_DECLARE(max_raymarch_steps_)
        FIELD_DECLARE(max_visible_distance_)
        FIELD_DECLARE(env_color_)
        FIELD_DECLARE(env_color_scale_)
        FIELD_DECLARE(sun_illuminance_scale_)
        FIELD_DECLARE(shadow_steps_)
        FIELD_DECLARE(shadow_distance_)
        FIELD_DECLARE(shadow_map_max_distance)
        FIELD_DECLARE(shadow_froxel_max_distance)
        FIELD_DECLARE(sun_multiscattering_sigma_scale)
        FIELD_DECLARE(env_multiscattering_sigma_scale)
    FIELD_DECLARATION_END()

    VolumetricCloud();

    struct {
        GLuint shadow_map;
        GLuint sampler;
        glm::mat4 light_vp_inv_model;
    } GetShadowMap() const {
        return { shadow_maps_[2].id(), shadow_map_sampler_.id(), light_vp_inv_model_ };
    }

    struct {
        GLuint shadow_froxel;
        GLuint sampler;
        float inv_max_distance;
    } GetShadowFroxel() const {
        return { viewport_data_->shadow_froxel.id(), Samplers::GetLinearNoMipmapClampToEdge(), 1.0f / shadow_froxel_max_distance };
    }

    void SetViewport(int width, int height);

    void Update(const Camera& camera, const Earth& earth, const AtmosphereRenderer& atmosphere_render);

    void RenderShadow();

    void Render(GLuint hdr_texture, GLuint depth_texture);

    void DrawGUI();

private:
    template<GLuint first = 0, GLsizei N> static void GLBindTextures(const GLuint(&arr)[N]) {
        static_assert(first + N <= IVolumetricCloudMaterial::kMaterialTextureUnitBegin);
        ::GLBindTextures(arr, first);
    }
    template<GLuint first = 0, GLsizei N> static void GLBindSamplers(const GLuint(&arr)[N]) {
        static_assert(first + N <= IVolumetricCloudMaterial::kMaterialTextureUnitBegin);
        ::GLBindSamplers(arr, first);
    }

    IVolumetricCloudMaterial* preframe_material_;

    struct ViewportData {
        ViewportData(glm::ivec2 viewport);

        GLTexture checkerboard_depth_;
        GLTexture index_linear_depth_;
        GLTexture render_texture_;
        GLTexture cloud_distance_texture_;
        GLTexture reconstruct_texture_[2];

        GLTexture shadow_froxel;
    };

    glm::ivec2 viewport_{};
    std::unique_ptr<ViewportData> viewport_data_;

    GLSampler shadow_map_sampler_;
    GLTexture shadow_maps_[3]; // current raw, previous raw, current 

    GLReloadableComputeProgram shadow_map_gen_program_;
    GLReloadableComputeProgram shadow_map_blur_program_[2];
    GLReloadableComputeProgram shadow_froxel_gen_program_;

    GLReloadableComputeProgram checkerboard_gen_program_;
    GLReloadableComputeProgram index_gen_program_;
    GLReloadableComputeProgram render_program_;
    GLReloadableComputeProgram reconstruct_program_;
    GLReloadableComputeProgram upscale_program_;

    GLBuffer common_buffer_;
    GLBuffer buffer_;

    glm::vec3 camera_pos_{0.0f, 0.0f, 0.0f};
    glm::mat4 mvp_ = glm::identity<glm::mat4>();
    glm::dvec2 offset_from_first_{ 0, 0 };
    int frame_id_ = 0;
    glm::mat4 model_ = glm::identity<glm::mat4>();
    glm::mat4 light_vp_ = glm::identity<glm::mat4>();

    glm::mat4 light_vp_inv_model_ = glm::identity<glm::mat4>();

    GLuint atmosphere_transmittance_tex_ = 0;
    GLuint aerial_perspective_luminance_tex_ = 0;
    GLuint aerial_perspective_transmittance_tex_ = 0;
};
