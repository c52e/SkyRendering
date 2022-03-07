#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "Singleton.h"
#include "GLProgram.h"

class GBuffer {
public:
	GBuffer(int width, int height);
	~GBuffer();

	GLuint id() const { return id_; }
	GLuint normal_texture() const { return normal_texture_; }
	GLuint albedo_texture() const { return albedo_texture_; }
	GLuint depth_stencil_texture() const { return depth_stencil_texture_; }

private:
	void Clean();

	GLuint id_;
	GLuint normal_texture_;
	GLuint albedo_texture_;
	GLuint depth_stencil_texture_;
};

inline void Clear(const GBuffer& gbuffer) {
	static const float black[] = { 0.f, 0.f, 0.f, 0.f };
	glClearNamedFramebufferfv(gbuffer.id(), GL_COLOR, 0, black);
	glClearNamedFramebufferfv(gbuffer.id(), GL_COLOR, 1, black);
	glClearNamedFramebufferfi(gbuffer.id(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
}


class GBufferRenderer : public Singleton<GBufferRenderer> {
public:
	friend Singleton<GBufferRenderer>;

	void Setup(const glm::mat4 model_matrix, const glm::mat4& view_projection_matrix, glm::vec3 albedo);

	void SetupWithTexture(const glm::mat4 model_matrix, const glm::mat4& view_projection_matrix, GLuint albedo_texture);

private:
	GBufferRenderer();

	std::unique_ptr<GLProgram> const_albedo_program_;
	std::unique_ptr<GLProgram> albedo_texture_program_;
};
