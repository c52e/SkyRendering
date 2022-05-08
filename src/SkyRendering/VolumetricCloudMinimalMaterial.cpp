#include "VolumetricCloudMinimalMaterial.h"

#include <imgui.h>

struct VolumetricCloudMinimalMaterial::BufferData {
	glm::vec3 padding;
	float uDensity;
};

VolumetricCloudMinimalMaterial::VolumetricCloudMinimalMaterial() {
	buffer_.Create();
	glNamedBufferStorage(buffer_.id(), sizeof(BufferData), NULL, GL_DYNAMIC_STORAGE_BIT);
}

std::string VolumetricCloudMinimalMaterial::ShaderPath() {
	return "../shaders/SkyRendering/VolumetricCloudMaterialMinimal.comp";
}

void VolumetricCloudMinimalMaterial::Setup(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) {
	BufferData buffer;
	buffer.uDensity = density_;

	glNamedBufferSubData(buffer_.id(), 0, sizeof(buffer), &buffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, buffer_.id());
}

void VolumetricCloudMinimalMaterial::DrawGUI() {
	ImGui::SliderFloat("Density", &density_, 0.0f, 1.0f);
}
