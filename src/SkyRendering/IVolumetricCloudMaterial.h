#pragma once

#include <glm/glm.hpp>

#include "gl.hpp"
#include "Camera.h"
#include "Serialization.h"

class IVolumetricCloudMaterial : public ISerializable {
public:
    static constexpr GLuint kMaterialTextureUnitBegin = 6;

    virtual ~IVolumetricCloudMaterial() = default;

    virtual std::string ShaderPath() = 0;

    virtual void Setup(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) = 0;

    virtual void DrawGUI() = 0;

protected:
    FIELD_DECLARATION_BEGIN(ISerializable)
    FIELD_DECLARATION_END()

    struct SampleInfo {
        glm::vec2 bias;
        float frequency;
        float k_lod;
    };

    struct TextureWithInfo {
        GLuint id() { return tex.id(); }

        GLTexture tex;
        float repeat_size = 20.0f;
        int x = 1;
        int y = 1;
        int z = 1;
        int channel = 1;
    };

    static float CalKLod(const TextureWithInfo& tex, glm::vec2 viewport, const Camera& camera);
};

HAS_SUBCLASS(IVolumetricCloudMaterial)
