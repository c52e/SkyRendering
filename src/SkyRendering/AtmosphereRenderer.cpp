#include "AtmosphereRenderer.h"

#include <array>

#include <glm/gtc/type_ptr.hpp>

#include "Utils.h"
#include "Textures.h"
#include "Samplers.h"
#include "ScreenRectangle.h"

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

struct AtmosphereRenderBufferData {
    glm::vec3 sun_direction;
    float star_luminance_scale;
    glm::vec3 earth_center;
    float camera_earth_center_distance;
    glm::vec3 camera_position;
    float raymarching_steps;
    glm::vec3 up_direction;
    float sky_view_lut_steps;
    glm::vec3 right_direction;
    float aerial_perspective_lut_steps;
    glm::vec3 front_direction;
    float aerial_perspective_lut_max_distance;
    glm::vec3 moon_position;
    float moon_radius;

    glm::mat4 inv_view_projection;
    glm::mat4 light_view_projection;

    glm::vec2 padding00;
    float blocker_kernel_size_k;
    float pcss_size_k;
};

static void AssignBufferData(const AtmosphereRenderParameters& parameters, const Earth& earth, AtmosphereRenderBufferData& data) {
    FromThetaPhiToDirection(glm::radians(parameters.sun_direction_theta), 
         glm::radians(parameters.sun_direction_phi), glm::value_ptr(data.sun_direction));
    data.star_luminance_scale = parameters.star_luminance_scale;
    data.camera_position = parameters.camera_position;
    data.inv_view_projection = glm::inverse(parameters.view_projection);
    data.light_view_projection = parameters.light_view_projection;
    data.raymarching_steps = parameters.raymarching_steps;
    data.sky_view_lut_steps = parameters.sky_view_lut_steps;
    data.aerial_perspective_lut_steps = parameters.aerial_perspective_lut_steps;
    data.aerial_perspective_lut_max_distance = parameters.aerial_perspective_lut_max_distance;
    data.moon_position = glm::vec3(earth.moon_model()[3]);
    data.moon_radius = earth.moon_status.radius;
    data.blocker_kernel_size_k = parameters.blocker_kernel_size_k;
    data.pcss_size_k = parameters.pcss_size_k;

    data.earth_center = earth.center();
    data.camera_earth_center_distance = glm::distance(data.camera_position, data.earth_center);
    data.up_direction = glm::normalize(data.camera_position - data.earth_center);
    data.right_direction = glm::cross(data.sun_direction, data.up_direction);
    float right_direction_length = glm::length(data.right_direction);
    if (right_direction_length != 0.0) {
        data.right_direction /= right_direction_length;
    }
    else {
        glm::vec3 sun_direction_biased;
        FromThetaPhiToDirection( glm::radians(parameters.sun_direction_theta + 90.f),
            glm::radians(parameters.sun_direction_phi), glm::value_ptr(sun_direction_biased));
        data.right_direction = glm::normalize(glm::cross(sun_direction_biased, data.up_direction));
    }
    data.front_direction = glm::cross(data.up_direction, data.right_direction);
}

AtmosphereRenderer::AtmosphereRenderer(const AtmosphereRenderInitParameters& init_parameters)
    : use_sky_view_lut_(init_parameters.use_sky_view_lut), use_aerial_perspective_lut_(init_parameters.use_aerial_perspective_lut) {
    atmosphere_render_buffer_.Create();
    glNamedBufferStorage(atmosphere_render_buffer_.id(), sizeof(AtmosphereRenderBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

    auto generate_shader_str = [&init_parameters] (const std::string& header, bool dither_sample_point_enable) {
        return "#version 460\n"
            + std::string(header)
            + "#define SKY_VIEW_LUT_SIZE ivec2(" + std::to_string(kSkyViewTextureWidth)
            + "," + std::to_string(kSkyViewTextureHeight) + ")\n"
            + "#define AERIAL_PERSPECTIVE_LUT_SIZE ivec3(" + std::to_string(kAerialPerspectiveTextureWidth)
            + "," + std::to_string(kAerialPerspectiveTextureHeight)
            + "," + std::to_string(kAerialPerspectiveTextureDepth) + ")\n"
            + "#define PCSS_ENABLE " + (init_parameters.pcss_enable ? "1\n" : "0\n")
            + "#define VOLUMETRIC_LIGHT_ENABLE " + (init_parameters.volumetric_light_enable ? "1\n" : "0\n")
            + "#define MOON_SHADOW_ENABLE " + (init_parameters.moon_shadow_enable ? "1\n" : "0\n")
            + "#define DITHER_SAMPLE_POINT_ENABLE " + (dither_sample_point_enable ? "1\n" : "0\n")
            + "#define USE_SKY_VIEW_LUT " + (init_parameters.use_sky_view_lut ? "1\n" : "0\n")
            + "#define USE_AERIAL_PERSPECTIVE_LUT " + (init_parameters.use_aerial_perspective_lut ? "1\n" : "0\n")
            + ReadWithPreprocessor("../shaders/SkyRendering/AtmosphereRenderer.glsl");
    };

    auto sky_view_compute_str = generate_shader_str(
        "#define SKY_VIEW_COMPUTE_PROGRAM\n"
        "#define LOCAL_SIZE_X " + std::to_string(kSkyViewLocalSizeX) + "\n"
        "#define LOCAL_SIZE_Y " + std::to_string(kSkyViewLocalSizeY) + "\n",
        init_parameters.sky_view_lut_dither_sample_point_enable);
    sky_view_program_ = GLProgram(sky_view_compute_str.c_str());

    sky_view_luminance_texture_.Create(GL_TEXTURE_2D);
    glTextureStorage2D(sky_view_luminance_texture_.id(), 1, kSkyViewTextureInternalFormat,
        kSkyViewTextureWidth, kSkyViewTextureHeight);

    sky_view_transmittance_texture_.Create(GL_TEXTURE_2D);
    glTextureStorage2D(sky_view_transmittance_texture_.id(), 1, kSkyViewTextureInternalFormat,
        kSkyViewTextureWidth, kSkyViewTextureHeight);

    auto aerial_perspective_compute_str = generate_shader_str(
        "#define AERIAL_PERSPECTIVE_COMPUTE_PROGRAM\n"
        "#define LOCAL_SIZE_X " + std::to_string(kAerialPerspectiveLocalSizeX) + "\n"
        "#define LOCAL_SIZE_Y " + std::to_string(kAerialPerspectiveLocalSizeY) + "\n"
        "#define LOCAL_SIZE_Z " + std::to_string(kAerialPerspectiveLocalSizeZ) + "\n",
        init_parameters.aerial_perspective_lut_dither_sample_point_enable);
    aerial_perspective_program_ = GLProgram(aerial_perspective_compute_str.c_str());

    aerial_perspective_luminance_texture_.Create(GL_TEXTURE_3D);
    glTextureStorage3D(aerial_perspective_luminance_texture_.id(), 1, kAerialPerspectiveTextureInternalFormat,
        kAerialPerspectiveTextureWidth, kAerialPerspectiveTextureHeight, kAerialPerspectiveTextureDepth);

    aerial_perspective_transmittance_texture_.Create(GL_TEXTURE_3D);
    glTextureStorage3D(aerial_perspective_transmittance_texture_.id(), 1, kAerialPerspectiveTextureInternalFormat,
        kAerialPerspectiveTextureWidth, kAerialPerspectiveTextureHeight, kAerialPerspectiveTextureDepth);

    auto atmosphere_render_fragment_str = generate_shader_str(
        "#define ATMOSPHERE_RENDER_FRAGMENT_SHADER\n" + ReadWithPreprocessor("../shaders/SkyRendering/Shadow.glsl") + "\n",
        init_parameters.raymarching_dither_sample_point_enable);
    render_program_ = GLProgram(kCommonVertexSrc, atmosphere_render_fragment_str.c_str());
}

void AtmosphereRenderer::Render(const Earth& earth, const AtmosphereRenderParameters& parameters) {
    AtmosphereRenderBufferData atmosphere_render_buffer_data_;
    AssignBufferData(parameters, earth, atmosphere_render_buffer_data_);
    sun_direction_ = atmosphere_render_buffer_data_.sun_direction;
    aerial_perspective_lut_max_distance_ = atmosphere_render_buffer_data_.aerial_perspective_lut_max_distance;

    glNamedBufferSubData(atmosphere_render_buffer_.id(), 0, sizeof(atmosphere_render_buffer_data_), &atmosphere_render_buffer_data_);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, atmosphere_render_buffer_.id());

    GLBindTextures({ earth.atmosphere().transmittance_texture(),
                    earth.atmosphere().multiscattering_texture(),
                    parameters.depth_stencil_texture,
                    parameters.albedo,
                    parameters.normal,
                    parameters.orm,
                    parameters.shadow_map_texture,
                    Textures::Instance().blue_noise(),
                    Textures::Instance().star_luminance(),
                    sky_view_luminance_texture_.id(),
                    sky_view_transmittance_texture_.id(),
                    aerial_perspective_luminance_texture_.id(),
                    aerial_perspective_transmittance_texture_.id(),
                    parameters.shadow_map_texture });
    GLBindSamplers({ Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    0u,
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetShadowMapSampler(),
                    0u,
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetLinearNoMipmapClampToEdge(),
                    Samplers::GetNearestClampToEdge(), });

    if (use_sky_view_lut_) {
        PERF_MARKER("SkyViewLut")
        GLBindImageTextures({ sky_view_luminance_texture_.id(), sky_view_transmittance_texture_.id() });
        glUseProgram(sky_view_program_.id());
        glDispatchCompute(kSkyViewProgramGlobalSizeX, kSkyViewProgramGlobalSizeY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    if (true) { // For Volumetric Cloud
        PERF_MARKER("AerialPerspective")
        GLBindImageTextures({ aerial_perspective_luminance_texture_.id(), aerial_perspective_transmittance_texture_.id() });
        glUseProgram(aerial_perspective_program_.id());
        glDispatchCompute(kAerialPerspectiveProgramGlobalSizeX, kAerialPerspectiveProgramGlobalSizeY, kAerialPerspectiveProgramGlobalSizeZ);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    {
        PERF_MARKER("Render")
        glUseProgram(render_program_.id());
        ScreenRectangle::Instance().Draw();
    }
}
