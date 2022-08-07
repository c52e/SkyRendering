#include "Atmosphere.h"

#include <array>

#include "Utils.h"
#include "Textures.h"
#include "Samplers.h"

constexpr GLuint kTransmittanceLocalSizeX = 16;
constexpr GLuint kTransmittanceLocalSizeY = 8;
constexpr GLsizei kTransmittanceTextureWidth = 256;
constexpr GLsizei kTransmittanceTextureHeight = 64;
constexpr GLenum kTransmittanceTextureInternalFormat = GL_RGBA32F;
constexpr GLuint kTransmittanceProgramGlobalSizeX = kTransmittanceTextureWidth / kTransmittanceLocalSizeX;
constexpr GLuint kTransmittanceProgramGlobalSizeY = kTransmittanceTextureHeight / kTransmittanceLocalSizeY;

constexpr GLuint kMultiscatteringTextureWidth = 32;
constexpr GLuint kMultiscatteringTextureHeight = 32;
constexpr GLenum kMultiscatteringTextureInternalFormat = GL_RGBA32F;

struct AtmosphereBufferData {
    glm::vec3 solar_illuminance;
    float sun_angular_radius;

    glm::vec3 rayleigh_scattering;
    float inv_rayleigh_exponential_distribution;

    glm::vec3 mie_scattering;
    float inv_mie_exponential_distribution;

    glm::vec3 mie_absorption;
    float ozone_center_altitude;

    glm::vec3 ozone_absorption;
    float inv_ozone_width;

    glm::vec3 ground_albedo;
    float mie_phase_g;

    glm::vec3 _atmosphere_padding;
    float multiscattering_mask;

    float bottom_radius;
    float top_radius;
    float transmittance_steps;
    float multiscattering_steps;
};

static void AssignBufferData(const AtmosphereParameters& parameters, AtmosphereBufferData& data) {
    data.solar_illuminance = parameters.solar_illuminance;
    data.sun_angular_radius = glm::radians(parameters.sun_angular_radius);

    data.rayleigh_scattering = parameters.rayleigh_scattering * parameters.rayleigh_scattering_scale;
    data.mie_scattering = parameters.mie_scattering * parameters.mie_scattering_scale;
    data.mie_absorption = parameters.mie_absorption * parameters.mie_absorption_scale;
    data.ozone_absorption = parameters.ozone_absorption * parameters.ozone_absorption_scale;

    data.inv_rayleigh_exponential_distribution = 1.0f / parameters.rayleigh_exponential_distribution;
    data.inv_mie_exponential_distribution = 1.0f / parameters.mie_exponential_distribution;
    data.ozone_center_altitude = parameters.ozone_center_altitude;
    data.inv_ozone_width = 1.0f / parameters.ozone_width;
    data.ground_albedo = parameters.ground_albedo;
    data.bottom_radius = parameters.bottom_radius;
    data.top_radius = parameters.bottom_radius + parameters.thickness;
    data.mie_phase_g = parameters.mie_phase_g;

    data.transmittance_steps = parameters.transmittance_steps;
    data.multiscattering_steps = parameters.multiscattering_steps;
    data.multiscattering_mask = parameters.multiscattering_mask;
}

Atmosphere::Atmosphere() {
    atmosphere_parameters_buffer_.Create();
    glNamedBufferStorage(atmosphere_parameters_buffer_.id(), sizeof(AtmosphereBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

    transmittance_texture_.Create(GL_TEXTURE_2D);
    glTextureStorage2D(transmittance_texture_.id(), 1, kTransmittanceTextureInternalFormat,
        kTransmittanceTextureWidth, kTransmittanceTextureHeight);

    transmittance_program_ = []() {
        auto transmittance_program_src =
            "#version 460\n"
            "#define TRANSMITTANCE_COMPUTE_PROGRAM\n"
            "#define LOCAL_SIZE_X " + std::to_string(kTransmittanceLocalSizeX) + "\n"
            "#define LOCAL_SIZE_Y " + std::to_string(kTransmittanceLocalSizeY) + "\n"
            + ReadWithPreprocessor("../shaders/SkyRendering/Atmosphere.glsl");
        return GLProgram(transmittance_program_src.c_str()); };

    multiscattering_texture_.Create(GL_TEXTURE_2D);
    glTextureStorage2D(multiscattering_texture_.id(), 1, kMultiscatteringTextureInternalFormat,
        kMultiscatteringTextureWidth, kMultiscatteringTextureHeight);

    multiscattering_program_ = []() {
        auto multiscattering_program_src =
            "#version 460\n"
            "#define MULTISCATTERING_COMPUTE_PROGRAM\n"
            + ReadWithPreprocessor("../shaders/SkyRendering/Atmosphere.glsl");
        return GLProgram(multiscattering_program_src.c_str()); };
}

void Atmosphere::UpdateLuts(const AtmosphereParameters& parameters) {
    PERF_MARKER("UpdateLuts")
    AtmosphereBufferData atmosphere_buffer_data_;
    AssignBufferData(parameters, atmosphere_buffer_data_);

    glNamedBufferSubData(atmosphere_parameters_buffer_.id(), 0, sizeof(atmosphere_buffer_data_), &atmosphere_buffer_data_);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, atmosphere_parameters_buffer_.id());

    GLBindImageTextures({ transmittance_texture_.id() });
    glUseProgram(transmittance_program_.id());
    {
        PERF_MARKER("Transmittance")
        glDispatchCompute(kTransmittanceProgramGlobalSizeX, kTransmittanceProgramGlobalSizeY, 1);
    }
    {
        PERF_MARKER("Multiscattering")
        GLBindImageTextures({ multiscattering_texture_.id() });
        GLBindTextures({ transmittance_texture() });
        GLBindSamplers({ Samplers::GetLinearNoMipmapClampToEdge() });
        glUseProgram(multiscattering_program_.id());
        glDispatchCompute(kMultiscatteringTextureWidth, kMultiscatteringTextureHeight, 1);
    }
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}
