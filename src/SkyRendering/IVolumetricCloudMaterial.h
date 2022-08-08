#pragma once

#include <glm/glm.hpp>

#include "gl.hpp"
#include "Camera.h"
#include "Serialization.h"

class IVolumetricCloudMaterial : public ISerializable {
public:
    static constexpr GLuint kMaterialTextureUnitBegin = 7;

    virtual ~IVolumetricCloudMaterial() = default;

    virtual std::string ShaderPath() = 0;

    virtual void Update(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) {}

    virtual void Bind() = 0;

    virtual void DrawGUI() = 0;

    virtual float GetSigmaTMax() = 0;

protected:
    FIELD_DECLARATION_BEGIN(ISerializable)
    FIELD_DECLARATION_END()
};

HAS_SUBCLASS(IVolumetricCloudMaterial)
