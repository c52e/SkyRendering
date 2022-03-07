#include "ShadowMap.h"

#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

#include "Utils.h"

ShadowMap::ShadowMap(int width, int height)
	: width_(width), height_(height) {
	glCreateFramebuffers(1, &framebuffer_id_);
	glCreateTextures(GL_TEXTURE_2D, 1, &depth_texture_);
	glTextureStorage2D(depth_texture_, 1, GL_DEPTH_COMPONENT32F, width, height);
	glNamedFramebufferTexture(framebuffer_id_, GL_DEPTH_ATTACHMENT, depth_texture_, 0);

	if (glCheckNamedFramebufferStatus(framebuffer_id_, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		Clean();
		throw std::runtime_error("Shadow Map Framebuffer Incomplete");
	}
}

ShadowMap::~ShadowMap() {
	Clean();
}

void ShadowMap::ClearBindViewport() const {
	glClearNamedFramebufferfi(framebuffer_id_, GL_DEPTH_STENCIL, 0, 1.0f, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
	glViewport(0, 0, width_, height_);
}

void ShadowMap::Clean() {
	glDeleteFramebuffers(1, &framebuffer_id_);
	glDeleteTextures(1, &depth_texture_);
}

static const char* kShadowMapRendererVertexSrc = R"(
#version 460
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 0) uniform mat4 mvp;
void main() {
	gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* kShadowMapRendererFragmentSrc = R"(
#version 460
void main() {
}
)";

ShadowMapRenderer::ShadowMapRenderer() {
	program_ = std::make_unique<GLProgram>(
		kShadowMapRendererVertexSrc, kShadowMapRendererFragmentSrc);
}

void ShadowMapRenderer::Setup(const glm::mat4 model_matrix, const glm::mat4 light_view_projection_matrix) {
	glUseProgram(program_->id());
	auto mvp = light_view_projection_matrix * model_matrix;
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
}

glm::mat4 ComputeLightMatrix(float center_distance, float theta, float phi,
		float left, float right, float bottom, float top, float near, float far) {
	glm::vec3 light_direction;
	FromThetaPhiToDirection(glm::radians(theta), glm::radians(phi), glm::value_ptr(light_direction));
	auto position = light_direction * center_distance;
	auto right_direction = glm::normalize(glm::cross(light_direction, glm::vec3(0, 1, 0)));
	auto up_direction = light_direction.y == 1.0f ? glm::vec3(1, 0, 0):
		glm::normalize(glm::cross(right_direction, light_direction));
	auto view_matrix = glm::lookAt(position, position - light_direction, up_direction);
	auto projection_matrix = glm::ortho(left, right, bottom, top, near, far);
	
	return projection_matrix * view_matrix;
}
