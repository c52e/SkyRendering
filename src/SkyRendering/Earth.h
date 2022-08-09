#pragma once

#include <glm/glm.hpp>

#include "gl.hpp"
#include "GLReloadableProgram.h"
#include "Camera.h"
#include "Atmosphere.h"

class Earth : public ISerializable {
public:
    AtmosphereParameters parameters;

    struct MoonStatus : public ISerializable {
        float direction_theta = 70.0f;
        float direction_phi = 150.0f;
        float distance = 384401.f / 10.f;
        float radius = 1737.f;

        FIELD_DECLARATION_BEGIN(ISerializable)
            FIELD_DECLARE(direction_theta)
            FIELD_DECLARE(direction_phi)
            FIELD_DECLARE(distance)
            FIELD_DECLARE(radius)
        FIELD_DECLARATION_END()
    } moon_status;

    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(parameters)
        FIELD_DECLARE(moon_status)
    FIELD_DECLARATION_END()

    Earth();

    void Update();

    glm::vec3 center() const { return { 0.0f, -parameters.bottom_radius, 0.0f }; }

    void RenderToGBuffer(const Camera& camera, GLuint depth_texture);

    const Atmosphere& atmosphere() const { return atmosphere_; }

    glm::mat4 moon_model() const;

private:
    Atmosphere atmosphere_;

    GLBuffer buffer_;
    GLReloadableProgram program_;
    GLSampler sampler_;
};
