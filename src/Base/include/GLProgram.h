#pragma once

#include <glad/glad.h>

class GLProgram {
public:
	GLProgram(const char* vertex_src, const char* fragment_src, GLuint external_fragment_shader = 0);
	GLProgram(const char* compute_src, GLuint external_compute_shader = 0);
	~GLProgram();

	GLuint id() const { return id_; }

private:
	GLuint id_ = 0;
};

