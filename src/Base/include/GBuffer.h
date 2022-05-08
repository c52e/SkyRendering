#pragma once

#include <glm/glm.hpp>

#include "gl.hpp"
#include "Singleton.h"
#include "GLReloadableProgram.h"
#include "MeshObject.h"

class GBuffer {
public:
	GBuffer(int width, int height);

	GLuint id() const { return framebuffer_.id(); }
	GLuint albedo() const { return albedo_.id(); }
	GLuint normal() const { return normal_.id(); }
	GLuint orm() const { return orm_.id(); }
	GLuint depth_stencil() const { return depth_stencil_.id(); }

private:
	GLFramebuffer framebuffer_;
	GLTexture albedo_;
	GLTexture normal_;
	GLTexture orm_;
	GLTexture depth_stencil_;
};

inline void Clear(const GBuffer& gbuffer) {
	static const float black[] = { 0.f, 0.f, 0.f, 0.f };
	glClearNamedFramebufferfv(gbuffer.id(), GL_COLOR, 0, black);
	glClearNamedFramebufferfv(gbuffer.id(), GL_COLOR, 1, black);
	glClearNamedFramebufferfv(gbuffer.id(), GL_COLOR, 2, black);
	glClearNamedFramebufferfi(gbuffer.id(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
}


class GBufferRenderer : public Singleton<GBufferRenderer> {
public:
	friend Singleton<GBufferRenderer>;

	void Setup(const glm::mat4 model_matrix, const glm::mat4& view_projection_matrix, const Material& material);

private:
	GBufferRenderer();

	GLReloadableProgram program_;
};
