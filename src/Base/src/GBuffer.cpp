#include "GBuffer.h"

#include <stdexcept>
#include <array>

#include <glm/gtc/type_ptr.hpp>

#include "Samplers.h"

GBuffer::GBuffer(int width, int height) {
	glCreateFramebuffers(1, &id_);
	glCreateTextures(GL_TEXTURE_2D, 1, &normal_texture_);
	glCreateTextures(GL_TEXTURE_2D, 1, &albedo_texture_);
	glCreateTextures(GL_TEXTURE_2D, 1, &depth_stencil_texture_);
 
	glTextureStorage2D(normal_texture_, 1, GL_RGBA32F, width, height); // RGB32FºÊ»›–‘≤Ó
	glTextureStorage2D(albedo_texture_, 1, GL_RGBA32F, width, height);
	glTextureStorage2D(depth_stencil_texture_, 1, GL_DEPTH24_STENCIL8, width, height);

	glNamedFramebufferTexture(id_, GL_COLOR_ATTACHMENT0, normal_texture_, 0);
	glNamedFramebufferTexture(id_, GL_COLOR_ATTACHMENT1, albedo_texture_, 0);
	glNamedFramebufferTexture(id_, GL_DEPTH_STENCIL_ATTACHMENT, depth_stencil_texture_, 0);

	GLenum attachments[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	constexpr GLsizei attachments_num = sizeof(attachments) / sizeof(attachments[0]);
	glNamedFramebufferDrawBuffers(id_, attachments_num, attachments);

	if (glCheckNamedFramebufferStatus(id_, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		Clean();
		throw std::runtime_error("GBuffer Incomplete");
	}
	
}

GBuffer::~GBuffer() {
	Clean();
}

void GBuffer::Clean() {
	glDeleteFramebuffers(1, &id_);
	glDeleteTextures(1, &normal_texture_);
	glDeleteTextures(1, &albedo_texture_);
	glDeleteTextures(1, &depth_stencil_texture_);
}


static const char* kGBufferRendererVertexSrc = R"(
#version 460
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
out vec3 vNormal;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform mat3 normal_matrix;
void main() {
	gl_Position = mvp * vec4(aPos, 1.0);
    vNormal = normalize(normal_matrix * aNormal);
}
)";

static const char* kGBufferRendererFragmentSrc = R"(
#version 460
in vec3 vNormal;
layout(location = 0) out vec4 Normal;
layout(location = 1) out vec4 Albedo;
layout(location = 2) uniform vec3 albedo;
void main() {
    Normal = vec4(vNormal, 1.0);
    Albedo = vec4(albedo, 1.0);
}
)";

static const char* kGBufferRendererWithTextureVertexSrc = R"(
#version 460
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
out vec3 vNormal;
out vec2 vUv;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform mat3 normal_matrix;
void main() {
	gl_Position = mvp * vec4(aPos, 1.0);
    vNormal = normalize(normal_matrix * aNormal);
	vUv = aUv;
}
)";

static const char* kGBufferRendererWithTextureFragmentSrc = R"(
#version 460
in vec3 vNormal;
in vec2 vUv;
layout(location = 0) out vec4 Normal;
layout(location = 1) out vec4 Albedo;
layout(binding = 0) uniform sampler2D albedo_texture;
void main() {
    Normal = vec4(vNormal, 1.0);
    Albedo = vec4(pow(texture(albedo_texture, vUv).xyz, vec3(2.2)), 1.0);
}
)";

GBufferRenderer::GBufferRenderer() {
	const_albedo_program_ = std::make_unique<GLProgram>(
		kGBufferRendererVertexSrc, kGBufferRendererFragmentSrc);
	albedo_texture_program_ = std::make_unique<GLProgram>(
		kGBufferRendererWithTextureVertexSrc, kGBufferRendererWithTextureFragmentSrc);
}

void GBufferRenderer::Setup(const glm::mat4 model_matrix,
		const glm::mat4& view_projection_matrix, glm::vec3 albedo) {
	glUseProgram(const_albedo_program_->id());
	auto mvp = view_projection_matrix * model_matrix;
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
	auto normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
	glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normal_matrix));
	glUniform3fv(2, 1, glm::value_ptr(albedo));
}

void GBufferRenderer::SetupWithTexture(const glm::mat4 model_matrix,
		const glm::mat4& view_projection_matrix, GLuint albedo_texture) {
	glUseProgram(albedo_texture_program_->id());
	auto mvp = view_projection_matrix * model_matrix;
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
	auto normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_matrix)));
	glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normal_matrix));
	std::array textures = { albedo_texture };
	glBindTextures(0, static_cast<GLsizei>(textures.size()), textures.data());
	std::array samplers = { Samplers::Instance().linear_clamp_to_edge() };
	glBindSamplers(0, static_cast<GLsizei>(samplers.size()), samplers.data());

}