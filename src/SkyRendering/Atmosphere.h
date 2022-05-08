#pragma once

#include <glm/glm.hpp>

#include "gl.hpp"
#include "GLProgram.h"
#include "PerformanceMarker.h"
#include "Serialization.h"

struct AtmosphereParameters : public ISerializable {
    // 角度单位：degree
    // 长度单位：km
    glm::vec3 solar_illuminance{ 1.f, 1.f, 1.f };
    float sun_angular_radius = 0.5334f * 0.5f;

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

    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(solar_illuminance)
        FIELD_DECLARE(sun_angular_radius)

        FIELD_DECLARE(bottom_radius)
        FIELD_DECLARE(thickness)
        FIELD_DECLARE(ground_albedo)

        FIELD_DECLARE(rayleigh_exponential_distribution)
        FIELD_DECLARE(rayleigh_scattering_scale)
        FIELD_DECLARE(rayleigh_scattering)

        FIELD_DECLARE(mie_exponential_distribution)
        FIELD_DECLARE(mie_phase_g)
        FIELD_DECLARE(mie_scattering_scale)
        FIELD_DECLARE(mie_scattering)
        FIELD_DECLARE(mie_absorption_scale)
        FIELD_DECLARE(mie_absorption)

        FIELD_DECLARE(ozone_center_altitude)
        FIELD_DECLARE(ozone_width)
        FIELD_DECLARE(ozone_absorption_scale)
        FIELD_DECLARE(ozone_absorption)

        FIELD_DECLARE(transmittance_steps)
        FIELD_DECLARE(multiscattering_steps)
        FIELD_DECLARE(multiscattering_mask)
    FIELD_DECLARATION_END()
};

class Atmosphere {
public:
    Atmosphere();

    void UpdateLuts(const AtmosphereParameters& parameters);

    GLuint transmittance_texture() const {
        return transmittance_texture_.id();
    }

    GLuint multiscattering_texture() const {
        return multiscattering_texture_.id();
    }

private:
    GLBuffer atmosphere_parameters_buffer_;

    GLTexture transmittance_texture_;
    GLProgram transmittance_program_;

    GLTexture multiscattering_texture_;
    GLProgram multiscattering_program_;
};
