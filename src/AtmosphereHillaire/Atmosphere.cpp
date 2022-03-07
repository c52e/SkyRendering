#include "Atmosphere.h"

#include <array>

#include <magic_enum.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Utils.h"
#include "Samplers.h"
#include "ScreenRectangle.h"

constexpr GLuint kTransmittanceLocalSizeX = 16;
constexpr GLuint kTransmittanceLocalSizeY = 2;
constexpr GLsizei kTransmittanceTextureWidth = 256;
constexpr GLsizei kTransmittanceTextureHeight = 64;
constexpr GLenum kTransmittanceTextureInternalFormat = GL_RGBA32F;
constexpr GLuint kTransmittanceProgramGlobalSizeX = kTransmittanceTextureWidth / kTransmittanceLocalSizeX;
constexpr GLuint kTransmittanceProgramGlobalSizeY = kTransmittanceTextureHeight / kTransmittanceLocalSizeY;

constexpr GLuint kMultiscatteringTextureWidth = 32;
constexpr GLuint kMultiscatteringTextureHeight = 32;
constexpr GLenum kMultiscatteringTextureInternalFormat = GL_RGBA32F;

constexpr GLuint kSkyViewLocalSizeX = 16;
constexpr GLuint kSkyViewLocalSizeY = 2;
constexpr GLsizei kSkyViewTextureWidth = 128;
constexpr GLsizei kSkyViewTextureHeight = 128;
constexpr GLenum kSkyViewTextureInternalFormat = GL_RGBA32F;
constexpr GLuint kSkyViewProgramGlobalSizeX = kSkyViewTextureWidth / kSkyViewLocalSizeX;
constexpr GLuint kSkyViewProgramGlobalSizeY = kSkyViewTextureHeight / kSkyViewLocalSizeY;

constexpr GLuint kAerialPerspectiveLocalSizeX = 8;
constexpr GLuint kAerialPerspectiveLocalSizeY = 4;
constexpr GLuint kAerialPerspectiveLocalSizeZ = 1;
constexpr GLsizei kAerialPerspectiveTextureWidth = 32;
constexpr GLsizei kAerialPerspectiveTextureHeight = 32;
constexpr GLsizei kAerialPerspectiveTextureDepth = 32;
constexpr GLenum kAerialPerspectiveTextureInternalFormat = GL_RGBA32F;
constexpr GLuint kAerialPerspectiveProgramGlobalSizeX = kAerialPerspectiveTextureWidth / kAerialPerspectiveLocalSizeX;
constexpr GLuint kAerialPerspectiveProgramGlobalSizeY = kAerialPerspectiveTextureHeight / kAerialPerspectiveLocalSizeY;
constexpr GLuint kAerialPerspectiveProgramGlobalSizeZ = kAerialPerspectiveTextureDepth / kAerialPerspectiveLocalSizeZ;


struct AtmosphereBufferData {
    glm::vec3 solar_illuminance;
    float sun_angular_radius;

    glm::vec3 rayleigh_scattering;
    float rayleigh_exponential_distribution;

    glm::vec3 mie_scattering;
    float mie_exponential_distribution;

    glm::vec3 mie_absorption;
    float ozone_center_altitude;

    glm::vec3 ozone_absorption;
    float ozone_width;

    glm::vec3 ground_albedo;
    float mie_phase_g;

    glm::vec3 earth_center;
    float multiscattering_mask;

    float bottom_radius;
    float top_radius;
    float transmittance_steps;
    float multiscattering_steps;
};

struct AtmosphereRenderBufferData {
    glm::vec3 sun_direction;
    float exposure;

    glm::vec3 camera_position;
    float star_luminance_scale;

    glm::mat4 inv_view_projection;
    glm::mat4 light_view_projection;

    glm::vec3 up_direction;
    float camera_earth_center_distance;
    glm::vec3 right_direction;
    float raymarching_steps;
    glm::vec3 front_direction;
    float sky_view_lut_steps;
    glm::vec2 padding10;
    float aerial_perspective_lut_steps;
    float aerial_perspective_lut_max_distance;
    glm::vec3 moon_position;
    float moon_radius;
};

static void AssignBufferData(const AtmosphereParameters& parameters, AtmosphereBufferData* data) {
    data->solar_illuminance = parameters.solar_illuminance;
    data->sun_angular_radius = glm::radians(parameters.sun_angular_radius);

    data->rayleigh_scattering = parameters.rayleigh_scattering * parameters.rayleigh_scattering_scale;
    data->mie_scattering = parameters.mie_scattering * parameters.mie_scattering_scale;
    data->mie_absorption = parameters.mie_absorption * parameters.mie_absorption_scale;
    data->ozone_absorption = parameters.ozone_absorption * parameters.ozone_absorption_scale;

    data->rayleigh_exponential_distribution = parameters.rayleigh_exponential_distribution;
    data->mie_exponential_distribution = parameters.mie_exponential_distribution;

    data->rayleigh_exponential_distribution = parameters.rayleigh_exponential_distribution;
    data->mie_exponential_distribution = parameters.mie_exponential_distribution;
    data->ozone_center_altitude = parameters.ozone_center_altitude;
    data->ozone_width = parameters.ozone_width;
    data->ground_albedo = parameters.ground_albedo;
    data->bottom_radius = parameters.bottom_radius;
    data->top_radius = parameters.bottom_radius + parameters.thickness;
    data->mie_phase_g = parameters.mie_phase_g;

    data->transmittance_steps = parameters.transmittance_steps;
    data->multiscattering_steps = parameters.multiscattering_steps;
    data->multiscattering_mask = parameters.multiscattering_mask;
    data->earth_center = glm::vec3(0, -parameters.bottom_radius, 0);
}

static void AssignBufferData(const AtmosphereRenderParameters& parameters, const Atmosphere& atmosphere, AtmosphereRenderBufferData* data) {
    FromThetaPhiToDirection(glm::radians(parameters.sun_direction_theta), 
         glm::radians(parameters.sun_direction_phi), glm::value_ptr(data->sun_direction));
    data->exposure = parameters.exposure;
    data->star_luminance_scale = parameters.star_luminance_scale;
    data->camera_position = parameters.camera_position;
    data->inv_view_projection = glm::inverse(parameters.view_projection);
    data->light_view_projection = parameters.light_view_projection;
    data->raymarching_steps = parameters.raymarching_steps;
    data->sky_view_lut_steps = parameters.sky_view_lut_steps;
    data->aerial_perspective_lut_steps = parameters.aerial_perspective_lut_steps;
    data->aerial_perspective_lut_max_distance = parameters.aerial_perspective_lut_max_distance;
    data->moon_position = parameters.moon_position;
    data->moon_radius = parameters.moon_radius;

    data->camera_earth_center_distance = glm::distance(data->camera_position, atmosphere.earth_center());
    data->up_direction = glm::normalize(data->camera_position - atmosphere.earth_center());
    data->right_direction = glm::cross(data->sun_direction, data->up_direction);
    float right_direction_length = glm::length(data->right_direction);
    if (right_direction_length != 0.0) {
        data->right_direction /= right_direction_length;
    }
    else {
        glm::vec3 sun_direction_biased;
        FromThetaPhiToDirection( glm::radians(parameters.sun_direction_theta + 90.f),
            glm::radians(parameters.sun_direction_phi), glm::value_ptr(sun_direction_biased));
        data->right_direction = glm::normalize(glm::cross(sun_direction_biased, data->up_direction));
    }
    data->front_direction = glm::cross(data->up_direction, data->right_direction);
}

#ifdef ATMOSPHERE_PROFILE
static double TimeElapsedMS(GLuint query) {
    /*GLint done = 0;
    while (!done)
        glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);*/
    GLuint64 elapsed_time;
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
    return static_cast<double>(elapsed_time) / 1e6;
}
#endif

Atmosphere::Atmosphere() {
#ifdef ATMOSPHERE_PROFILE
    glCreateQueries(GL_TIME_ELAPSED, 2, queries_);
#endif

    glCreateBuffers(1, &atmosphere_parameters_buffer_);
    glNamedBufferStorage(atmosphere_parameters_buffer_, sizeof(AtmosphereBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateTextures(GL_TEXTURE_2D, 1, &transmittance_texture_);
    glTextureStorage2D(transmittance_texture_, 1, kTransmittanceTextureInternalFormat,
        kTransmittanceTextureWidth, kTransmittanceTextureHeight);

    auto transmittance_program_src =
        "#version 460\n"
        "#define TRANSMITTANCE_COMPUTE_PROGRAM\n"
        "#define LOCAL_SIZE_X " + std::to_string(kTransmittanceLocalSizeX) + "\n"
        "#define LOCAL_SIZE_Y " + std::to_string(kTransmittanceLocalSizeY) + "\n"
        + ReadFile("../shaders/AtmosphereHillaire/atmosphere.glsl");
    transmittance_program_ = std::make_unique<GLProgram>(transmittance_program_src.c_str());

    glCreateTextures(GL_TEXTURE_2D, 1, &multiscattering_texture_);
    glTextureStorage2D(multiscattering_texture_, 1, kMultiscatteringTextureInternalFormat,
        kMultiscatteringTextureWidth, kMultiscatteringTextureHeight);

    auto multiscattering_program_src =
        "#version 460\n"
        "#define MULTISCATTERING_COMPUTE_PROGRAM\n"
        + ReadFile("../shaders/AtmosphereHillaire/atmosphere.glsl");
    multiscattering_program_ = std::make_unique<GLProgram>(multiscattering_program_src.c_str());
}

Atmosphere::~Atmosphere() {
#ifdef ATMOSPHERE_PROFILE
    glDeleteQueries(2, queries_);
#endif
    glDeleteBuffers(1, &atmosphere_parameters_buffer_);
    glDeleteTextures(1, &transmittance_texture_);
    glDeleteTextures(1, &multiscattering_texture_);
}

void Atmosphere::UpdateLuts(const AtmosphereParameters& parameters) {
    AtmosphereBufferData atmosphere_buffer_data_;
    AssignBufferData(parameters, &atmosphere_buffer_data_);
    earth_center_ = atmosphere_buffer_data_.earth_center;

    glNamedBufferSubData(atmosphere_parameters_buffer_, 0, sizeof(atmosphere_buffer_data_), &atmosphere_buffer_data_);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, atmosphere_parameters_buffer_);

    glBindImageTexture(0, transmittance_texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, kTransmittanceTextureInternalFormat);
    glUseProgram(transmittance_program_->id());
#ifdef ATMOSPHERE_PROFILE
    elapsed_ms_[0] = TimeElapsedMS(queries_[0]);
    glBeginQuery(GL_TIME_ELAPSED, queries_[0]);
#endif
    glDispatchCompute(kTransmittanceProgramGlobalSizeX, kTransmittanceProgramGlobalSizeY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef ATMOSPHERE_PROFILE
    glEndQuery(GL_TIME_ELAPSED);
    elapsed_ms_[1] = TimeElapsedMS(queries_[1]);
    glBeginQuery(GL_TIME_ELAPSED, queries_[1]);
#endif
    glBindImageTexture(0, multiscattering_texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, kMultiscatteringTextureInternalFormat);
    std::array textures = { transmittance_texture() };
    glBindTextures(0, static_cast<GLsizei>(textures.size()), textures.data());
    std::array samplers = { Samplers::Instance().linear_no_mipmap_clamp_to_edge() };
    glBindSamplers(0, static_cast<GLsizei>(samplers.size()), samplers.data());
    glUseProgram(multiscattering_program_->id());
    glDispatchCompute(kMultiscatteringTextureWidth, kMultiscatteringTextureHeight, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef ATMOSPHERE_PROFILE
    glEndQuery(GL_TIME_ELAPSED);
#endif
}

static const char* kAtmosphereRenderVertexSrc = R"(
#version 460
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
out vec2 vNDC;
void main() {
	vTexCoord = aPos * 0.5 + 0.5;
    vNDC = aPos;
	gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

AtmosphereRenderer::AtmosphereRenderer(const AtmosphereRenderInitParameters& init_parameters)
    : use_sky_view_lut_(init_parameters.use_sky_view_lut), use_aerial_perspective_lut_(init_parameters.use_aerial_perspective_lut) {
#ifdef ATMOSPHERE_PROFILE
    glCreateQueries(GL_TIME_ELAPSED, 3, queries_);
#endif

    glCreateBuffers(1, &atmosphere_render_buffer_);
    glNamedBufferStorage(atmosphere_render_buffer_, sizeof(AtmosphereRenderBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

    auto generate_shader_str = [&init_parameters] (const std::string& header, bool dither_sample_point_enable) {
        return "#version 460\n"
            + std::string(header)
            + "#define SKY_VIEW_LUT_SIZE ivec2(" + std::to_string(kSkyViewTextureWidth)
            + "," + std::to_string(kSkyViewTextureHeight) + ")\n"
            + "#define AERIAL_PERSPECTIVE_LUT_SIZE ivec3(" + std::to_string(kAerialPerspectiveTextureWidth)
            + "," + std::to_string(kAerialPerspectiveTextureHeight)
            + "," + std::to_string(kAerialPerspectiveTextureDepth) + ")\n"
            + "#define DITHER_COLOR_ENABLE " + (init_parameters.dither_color_enable ? "1\n" : "0\n")
            + "#define VOLUMETRIC_LIGHT_ENABLE " + (init_parameters.volumetric_light_enable ? "1\n" : "0\n")
            + "#define MOON_SHADOW_ENABLE " + (init_parameters.moon_shadow_enable ? "1\n" : "0\n")
            + "#define DITHER_SAMPLE_POINT_ENABLE " + (dither_sample_point_enable ? "1\n" : "0\n")
            + "#define USE_SKY_VIEW_LUT " + (init_parameters.use_sky_view_lut ? "1\n" : "0\n")
            + "#define USE_AERIAL_PERSPECTIVE_LUT " + (init_parameters.use_aerial_perspective_lut ? "1\n" : "0\n")
            + "#define TONE_MAPPING " + std::string(magic_enum::enum_name(init_parameters.tone_mapping)) + "\n"
            + ReadFile("../shaders/AtmosphereHillaire/atmosphere.glsl")
            + "\n"
            + ReadFile("../shaders/AtmosphereHillaire/atmosphere_renderer.glsl");
    };

    auto sky_view_compute_str = generate_shader_str(
        "#define SKY_VIEW_COMPUTE_PROGRAM\n"
        "#define LOCAL_SIZE_X " + std::to_string(kSkyViewLocalSizeX) + "\n"
        "#define LOCAL_SIZE_Y " + std::to_string(kSkyViewLocalSizeY) + "\n",
        init_parameters.sky_view_lut_dither_sample_point_enable);
    sky_view_program_ = std::make_unique<GLProgram>(sky_view_compute_str.c_str());

    glCreateTextures(GL_TEXTURE_2D, 1, &sky_view_luminance_texture_);
    glTextureStorage2D(sky_view_luminance_texture_, 1, kSkyViewTextureInternalFormat,
        kSkyViewTextureWidth, kSkyViewTextureHeight);

    glCreateTextures(GL_TEXTURE_2D, 1, &sky_view_transmittance_texture_);
    glTextureStorage2D(sky_view_transmittance_texture_, 1, kSkyViewTextureInternalFormat,
        kSkyViewTextureWidth, kSkyViewTextureHeight);

    auto aerial_perspective_compute_str = generate_shader_str(
        "#define AERIAL_PERSPECTIVE_COMPUTE_PROGRAM\n"
        "#define LOCAL_SIZE_X " + std::to_string(kAerialPerspectiveLocalSizeX) + "\n"
        "#define LOCAL_SIZE_Y " + std::to_string(kAerialPerspectiveLocalSizeY) + "\n"
        "#define LOCAL_SIZE_Z " + std::to_string(kAerialPerspectiveLocalSizeZ) + "\n",
        init_parameters.aerial_perspective_lut_dither_sample_point_enable);
    aerial_perspective_program_ = std::make_unique<GLProgram>(aerial_perspective_compute_str.c_str());

    glCreateTextures(GL_TEXTURE_3D, 1, &aerial_perspective_luminance_texture_);
    glTextureStorage3D(aerial_perspective_luminance_texture_, 1, kAerialPerspectiveTextureInternalFormat,
        kAerialPerspectiveTextureWidth, kAerialPerspectiveTextureHeight, kAerialPerspectiveTextureDepth);

    glCreateTextures(GL_TEXTURE_3D, 1, &aerial_perspective_transmittance_texture_);
    glTextureStorage3D(aerial_perspective_transmittance_texture_, 1, kAerialPerspectiveTextureInternalFormat,
        kAerialPerspectiveTextureWidth, kAerialPerspectiveTextureHeight, kAerialPerspectiveTextureDepth);

    auto atmosphere_render_fragment_str = generate_shader_str(
        "#define ATMOSPHERE_RENDER_FRAGMENT_SHADER\n", init_parameters.raymarching_dither_sample_point_enable);
    render_program_ = std::make_unique<GLProgram>(
        kAtmosphereRenderVertexSrc, atmosphere_render_fragment_str.c_str());
}

AtmosphereRenderer::~AtmosphereRenderer() {
#ifdef ATMOSPHERE_PROFILE
    glDeleteQueries(3, queries_);
#endif
    glDeleteBuffers(1, &atmosphere_render_buffer_);

    glDeleteTextures(1, &sky_view_luminance_texture_);
    glDeleteTextures(1, &sky_view_transmittance_texture_);
    glDeleteTextures(1, &aerial_perspective_luminance_texture_);
    glDeleteTextures(1, &aerial_perspective_transmittance_texture_);
}

void AtmosphereRenderer::Render(const Atmosphere& atmosphere, const AtmosphereRenderParameters& parameters) {
    AtmosphereRenderBufferData atmosphere_render_buffer_data_;
    AssignBufferData(parameters, atmosphere, &atmosphere_render_buffer_data_);

    glNamedBufferSubData(atmosphere_render_buffer_, 0, sizeof(atmosphere_render_buffer_data_), &atmosphere_render_buffer_data_);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, atmosphere_render_buffer_);

    std::array textures = { atmosphere.transmittance_texture(),
                            atmosphere.multiscattering_texture(),
                            parameters.depth_stencil_texture,
                            parameters.normal_texture,
                            parameters.albedo_texture,
                            parameters.shadow_map_texture,
                            Textures::Instance().blue_noise_texture(),
                            Textures::Instance().star_texture(),
                            Textures::Instance().earth_texture(),
                            sky_view_luminance_texture_,
                            sky_view_transmittance_texture_,
                            aerial_perspective_luminance_texture_,
                            aerial_perspective_transmittance_texture_ };
    glBindTextures(0, static_cast<GLsizei>(textures.size()), textures.data());
    std::array samplers = { Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().shadow_map_sampler(),
                            static_cast<GLuint>(0),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge(),
                            Samplers::Instance().linear_no_mipmap_clamp_to_edge() };
    glBindSamplers(0, static_cast<GLsizei>(samplers.size()), samplers.data());

    if (use_sky_view_lut_) {
#ifdef ATMOSPHERE_PROFILE
        elapsed_ms_[0] = TimeElapsedMS(queries_[0]);
        glBeginQuery(GL_TIME_ELAPSED, queries_[0]);
#endif
        glBindImageTexture(0, sky_view_luminance_texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, kSkyViewTextureInternalFormat);
        glBindImageTexture(1, sky_view_transmittance_texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, kSkyViewTextureInternalFormat);
        glUseProgram(sky_view_program_->id());
        glDispatchCompute(kSkyViewProgramGlobalSizeX, kSkyViewProgramGlobalSizeY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

#ifdef ATMOSPHERE_PROFILE
        glEndQuery(GL_TIME_ELAPSED);
#endif
    }

    if (use_aerial_perspective_lut_) {
#ifdef ATMOSPHERE_PROFILE
        elapsed_ms_[1] = TimeElapsedMS(queries_[1]);
        glBeginQuery(GL_TIME_ELAPSED, queries_[1]);
#endif
        glBindImageTexture(0, aerial_perspective_luminance_texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, kAerialPerspectiveTextureInternalFormat);
        glBindImageTexture(1, aerial_perspective_transmittance_texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, kAerialPerspectiveTextureInternalFormat);
        glUseProgram(aerial_perspective_program_->id());
        glDispatchCompute(kAerialPerspectiveProgramGlobalSizeX, kAerialPerspectiveProgramGlobalSizeY, kAerialPerspectiveProgramGlobalSizeZ);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

#ifdef ATMOSPHERE_PROFILE
        glEndQuery(GL_TIME_ELAPSED);
#endif
    }

#ifdef ATMOSPHERE_PROFILE
    elapsed_ms_[2] = TimeElapsedMS(queries_[2]);
    glBeginQuery(GL_TIME_ELAPSED, queries_[2]);
#endif
    glUseProgram(render_program_->id());
    ScreenRectangle::Instance().Draw();

#ifdef ATMOSPHERE_PROFILE
    glEndQuery(GL_TIME_ELAPSED);
#endif
}
