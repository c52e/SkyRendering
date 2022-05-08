#pragma once

#include <glm/glm.hpp>

#include "gl.hpp"
#include "GLProgram.h"
#include "Earth.h"
#include "PerformanceMarker.h"
#include "Serialization.h"

struct AtmosphereRenderParameters : public ISerializable {
    float sun_direction_theta = 70.0f; // Ìì¶¥½Ç
    float sun_direction_phi = 180.0f; // ·½Î»½Ç
    float star_luminance_scale = 0.005f;
    float raymarching_steps = 40.f;
    float sky_view_lut_steps = 40.f;
    float aerial_perspective_lut_steps = 40.f;
    float aerial_perspective_lut_max_distance = 100.f;

    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(sun_direction_theta)
        FIELD_DECLARE(sun_direction_phi)
        FIELD_DECLARE(star_luminance_scale)
        FIELD_DECLARE(raymarching_steps)
        FIELD_DECLARE(sky_view_lut_steps)
        FIELD_DECLARE(aerial_perspective_lut_steps)
        FIELD_DECLARE(aerial_perspective_lut_max_distance)
    FIELD_DECLARATION_END()

    glm::vec3 camera_position{};
    glm::mat4 view_projection{};
    glm::mat4 light_view_projection{};
    float blocker_kernel_size_k{};
    float pcss_size_k{};

    GLuint depth_stencil_texture{};
    GLuint normal{};
    GLuint albedo{};
    GLuint orm{};
    GLuint shadow_map_texture{};
};

struct AtmosphereRenderInitParameters : public ISerializable {
    bool pcss_enable = true;
    bool volumetric_light_enable = true;
    bool moon_shadow_enable = false;
    bool raymarching_dither_sample_point_enable = true;
    bool use_sky_view_lut = false;
    bool sky_view_lut_dither_sample_point_enable = false;
    bool use_aerial_perspective_lut = false;
    bool aerial_perspective_lut_dither_sample_point_enable = false;

    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(pcss_enable)
        FIELD_DECLARE(volumetric_light_enable)
        FIELD_DECLARE(moon_shadow_enable)
        FIELD_DECLARE(raymarching_dither_sample_point_enable)
        FIELD_DECLARE(use_sky_view_lut)
        FIELD_DECLARE(sky_view_lut_dither_sample_point_enable)
        FIELD_DECLARE(use_aerial_perspective_lut)
        FIELD_DECLARE(aerial_perspective_lut_dither_sample_point_enable)
    FIELD_DECLARATION_END()
};

inline bool operator==(const AtmosphereRenderInitParameters& lhs, const AtmosphereRenderInitParameters& rhs) {
    return lhs.pcss_enable == rhs.pcss_enable
        && lhs.volumetric_light_enable == rhs.volumetric_light_enable
        && lhs.moon_shadow_enable == rhs.moon_shadow_enable
        && lhs.raymarching_dither_sample_point_enable == rhs.raymarching_dither_sample_point_enable
        && lhs.use_sky_view_lut == rhs.use_sky_view_lut
        && lhs.sky_view_lut_dither_sample_point_enable == rhs.sky_view_lut_dither_sample_point_enable
        && lhs.use_aerial_perspective_lut == rhs.use_aerial_perspective_lut
        && lhs.aerial_perspective_lut_dither_sample_point_enable == rhs.aerial_perspective_lut_dither_sample_point_enable;
}

inline bool operator!=(const AtmosphereRenderInitParameters& lhs, const AtmosphereRenderInitParameters& rhs) {
    return !(lhs == rhs);
}

class AtmosphereRenderer {
public:
    AtmosphereRenderer(const AtmosphereRenderInitParameters& init_parameters);

    void Render(const Earth& earth, const AtmosphereRenderParameters& parameters);

    glm::vec3 sun_direction() const {
        return sun_direction_;
    }

    struct {
        GLuint luminance_tex;
        GLuint transmittance_tex;
        float max_distance;
    } aerial_perspective_lut() const {
        return { aerial_perspective_luminance_texture_.id()
            , aerial_perspective_transmittance_texture_.id()
            , aerial_perspective_lut_max_distance_ };
    }

private:
    glm::vec3 sun_direction_{};
    float aerial_perspective_lut_max_distance_{};

    bool use_sky_view_lut_;
    bool use_aerial_perspective_lut_;

    GLBuffer atmosphere_render_buffer_;

    GLTexture sky_view_luminance_texture_;
    GLTexture sky_view_transmittance_texture_;
    GLTexture aerial_perspective_luminance_texture_;
    GLTexture aerial_perspective_transmittance_texture_;

    GLProgram sky_view_program_;
    GLProgram aerial_perspective_program_;
    GLProgram render_program_;
};
