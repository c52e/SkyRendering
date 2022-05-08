#include "IVolumetricCloudMaterial.h"

#include "VolumetricCloudDefaultMaterial.h"
#include "VolumetricCloudMinimalMaterial.h"

float IVolumetricCloudMaterial::CalKLod(const TextureWithInfo& tex, glm::vec2 viewport, const Camera& camera) {
	auto max_width = static_cast<float>(glm::max(tex.x, glm::max(tex.y, tex.z)));
	auto tan_half_fovy = glm::tan(camera.fovy * 0.5f);
	return max_width * tan_half_fovy / (tex.repeat_size * static_cast<float>(glm::min(viewport.x, viewport.y)));
}

SUBCLASS_DECLARATION_BEGIN(IVolumetricCloudMaterial)
SUBCLASS_DECLARATION(VolumetricCloudDefaultMaterial)
SUBCLASS_DECLARATION(VolumetricCloudMinimalMaterial)
SUBCLASS_DECLARATION_END()
