#pragma once

#include <memory>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "GLProgram.h"

#define ATMOSPHERE_PROFILE

struct AtmosphereParameters {
    // 角度单位：degree
    // 长度单位：km
    glm::vec3 solar_illuminance{ 1.f, 1.f, 1.f };
    float sun_angular_radius = 0.03f * 57.3f;

    float bottom_radius = 6360.0f;
    float thickness = 6420.0f - 6360.0f;
    glm::vec3 ground_albedo{ 0.4f, 0.4f, 0.4f };

    float rayleigh_exponential_distribution = 8.0f;
    float rayleigh_scattering_scale = 0.0331f;
    glm::vec3 rayleigh_scattering{ 0.175287f, 0.409607f, 1.0f };

    float mie_exponential_distribution = 1.2f;
    float mie_phase_g = 0.8f;
    float mie_scattering_scale = 0.003996f;
    glm::vec3 mie_scattering{ 1.0f, 1.0f, 1.0f };
    float mie_absorption_scale = 0.000444f;
    glm::vec3 mie_absorption{ 1.0f, 1.0f, 1.0f };

    float ozone_center_altitude = 25.0f;
    float ozone_width = 15.0f;
    float ozone_absorption_scale = 0.001881f;
    glm::vec3 ozone_absorption{ 0.345561f, 1.0f , 0.045189f };

    float transmittance_steps = 40.0f;
    float multiscattering_steps = 30.0f;
    float multiscattering_mask = 1.0f; // 是否考虑多重散射[0,1]
};

struct AtmosphereRenderParameters {
    float sun_direction_theta = 70.0f; // 天顶角
    float sun_direction_phi = 180.0f; // 方位角
    float exposure = 10.f;
    float star_luminance_scale = 0.005f;
    float raymarching_steps = 40.f;
    float sky_view_lut_steps = 40.f;
    float aerial_perspective_lut_steps = 40.f;
    float aerial_perspective_lut_max_distance = 31.f;

    glm::vec3 camera_position{};
    glm::mat4 view_projection{};
    glm::mat4 light_view_projection{};
    glm::vec3 moon_position{};
    float moon_radius{};

    GLuint depth_stencil_texture{};
    GLuint normal_texture{};
    GLuint albedo_texture{};
    GLuint shadow_map_texture{};
};

enum class ToneMapping {
    CEToneMapping,
    ACESToneMapping
};

struct AtmosphereRenderInitParameters {
    bool dither_color_enable = true;
    bool volumetric_light_enable = true;
    bool moon_shadow_enable = false;
    bool raymarching_dither_sample_point_enable = true;
    bool use_sky_view_lut = false;
    bool sky_view_lut_dither_sample_point_enable = false;
    bool use_aerial_perspective_lut = false;
    bool aerial_perspective_lut_dither_sample_point_enable = false;
    ToneMapping tone_mapping = ToneMapping::CEToneMapping;
};

inline bool operator==(const AtmosphereRenderInitParameters& lhs, const AtmosphereRenderInitParameters& rhs) {
    return lhs.dither_color_enable == rhs.dither_color_enable
        && lhs.volumetric_light_enable == rhs.volumetric_light_enable
        && lhs.moon_shadow_enable == rhs.moon_shadow_enable
        && lhs.raymarching_dither_sample_point_enable == rhs.raymarching_dither_sample_point_enable
        && lhs.use_sky_view_lut == rhs.use_sky_view_lut
        && lhs.sky_view_lut_dither_sample_point_enable == rhs.sky_view_lut_dither_sample_point_enable
        && lhs.use_aerial_perspective_lut == rhs.use_aerial_perspective_lut
        && lhs.aerial_perspective_lut_dither_sample_point_enable == rhs.aerial_perspective_lut_dither_sample_point_enable
        && lhs.tone_mapping == rhs.tone_mapping;
}

inline bool operator!=(const AtmosphereRenderInitParameters& lhs, const AtmosphereRenderInitParameters& rhs) {
    return !(lhs == rhs);
}

class Atmosphere {
public:
    Atmosphere();
    ~Atmosphere();

    void UpdateLuts(const AtmosphereParameters& parameters);

    GLuint transmittance_texture() const {
        return transmittance_texture_;
    }

    GLuint multiscattering_texture() const {
        return multiscattering_texture_;
    }

    glm::vec3 earth_center() const {
        return earth_center_;
    }
#ifdef ATMOSPHERE_PROFILE
public:
    double TransmittanceTimeElapsedMS() {
        return elapsed_ms_[0];
    }
    double MultiscatteringTimeElapsedMS() {
        return elapsed_ms_[1];
    }
private:
    GLuint queries_[2];
    double elapsed_ms_[2];
#endif

private:
    glm::vec3 earth_center_;

    GLuint atmosphere_parameters_buffer_;

    GLuint transmittance_texture_;
    std::unique_ptr<GLProgram> transmittance_program_;

    GLuint multiscattering_texture_;
    std::unique_ptr<GLProgram> multiscattering_program_;
};

class AtmosphereRenderer {
public:
    AtmosphereRenderer(const AtmosphereRenderInitParameters& init_parameters);
    ~AtmosphereRenderer();

    void Render(const Atmosphere& atmosphere, const AtmosphereRenderParameters& parameters);

#ifdef ATMOSPHERE_PROFILE
public:
    double SkyViewTimeElapsedMS() {
        return elapsed_ms_[0];
    }
    double AerialPerspectiveTimeElapsedMS() {
        return elapsed_ms_[1];
    }
    double RenderTimeElapsedMS() {
        return elapsed_ms_[2];
    }
private:
    GLuint queries_[3];
    double elapsed_ms_[3];
#endif

private:
    bool use_sky_view_lut_;
    bool use_aerial_perspective_lut_;

    GLuint atmosphere_render_buffer_;

    GLuint sky_view_luminance_texture_;
    GLuint sky_view_transmittance_texture_;
    GLuint aerial_perspective_luminance_texture_;
    GLuint aerial_perspective_transmittance_texture_;

    std::unique_ptr<GLProgram> sky_view_program_;
    std::unique_ptr<GLProgram> aerial_perspective_program_;
    std::unique_ptr<GLProgram> render_program_;
};
