#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "Singleton.h"
#include "GLProgram.h"

class ShadowMap {
public:
	ShadowMap(int width, int height);
	
	~ShadowMap();

	void ClearBindViewport() const;

	GLuint depth_texture() const { return depth_texture_; }

private:
	void Clean();

	int width_;
	int height_;

	GLuint framebuffer_id_;
	GLuint depth_texture_;
};

class ShadowMapRenderer : public Singleton<ShadowMapRenderer> {
public:
	friend Singleton<ShadowMapRenderer>;

	void Setup(const glm::mat4 model_matrix, const glm::mat4 light_view_projection_matrix);

private:
	ShadowMapRenderer();

	std::unique_ptr<GLProgram> program_;
};

glm::mat4 ComputeLightMatrix(float center_distance, float theta, float phi,
	float left, float right, float bottom, float top, float near, float far);
