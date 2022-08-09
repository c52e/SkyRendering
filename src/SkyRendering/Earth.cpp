#include "Earth.h"

#include <array>

#include <glm/gtc/matrix_transform.hpp>

#include "Textures.h"
#include "Samplers.h"
#include "ScreenRectangle.h"
#include "Atmosphere.h"

struct EarthBufferData {
    glm::mat4 view_projection;
    glm::mat4 inv_view_projection;
    glm::vec3 camera_position;
    float camera_earth_center_distance;
    glm::vec3 earth_center;
    float padding;
    glm::vec3 up_direction;
    float padding1;
};

Earth::Earth() {
    buffer_.Create();
    glNamedBufferStorage(buffer_.id(), sizeof(EarthBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

    program_ = []() {
        auto src = ReadWithPreprocessor("../shaders/SkyRendering/EarthRender.frag");
        return GLProgram(kCommonVertexSrc, src.c_str());
    };

    sampler_.Create();
    glSamplerParameteri(sampler_.id(), GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_.id(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_.id(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler_.id(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    float max_texture_max_anisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_texture_max_anisotropy);
    glSamplerParameterf(sampler_.id(), GL_TEXTURE_MAX_ANISOTROPY, max_texture_max_anisotropy);
}

void Earth::Update() {
    atmosphere_.UpdateLuts(parameters);
}

void Earth::RenderToGBuffer(const Camera& camera, GLuint depth_texture) {
    EarthBufferData buffer;
    buffer.view_projection = camera.ViewProjection();
    buffer.inv_view_projection = glm::inverse(buffer.view_projection);
    buffer.camera_position = camera.position();
    buffer.earth_center = center();
    buffer.camera_earth_center_distance = glm::distance(buffer.camera_position, buffer.earth_center);
    buffer.up_direction = glm::normalize(buffer.camera_position - buffer.earth_center);

    glNamedBufferSubData(buffer_.id(), 0, sizeof(buffer), &buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, buffer_.id());

    GLBindTextures({ depth_texture, Textures::Instance().earth_albedo() });
    GLBindSamplers({ 0u, sampler_.id() });

    glUseProgram(program_.id());
    glDepthFunc(GL_ALWAYS);
    ScreenRectangle::Instance().Draw();
    glDepthFunc(GL_LESS);
}

glm::mat4 Earth::moon_model() const {
    auto model = glm::identity<glm::mat4>();
    model = glm::translate(model, center());
    model = glm::rotate(model, glm::radians(moon_status.direction_phi), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(moon_status.direction_theta), glm::vec3(0, 0, -1));
    model = glm::translate(model,
        glm::vec3(0, moon_status.distance + moon_status.radius + parameters.bottom_radius, 0));
    model = glm::scale(model, glm::vec3(moon_status.radius));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0, 0, 1));
    return model;
}
