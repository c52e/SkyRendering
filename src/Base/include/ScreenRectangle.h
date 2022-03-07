#pragma once

#include <memory>

#include <glad/glad.h>

#include "Singleton.h"
#include "GLProgram.h"

class ScreenRectangle :public Singleton<ScreenRectangle> {
public:
	friend Singleton<ScreenRectangle>;

	static inline const float vertices[] = {
		-1.0f, 1.0f,
		1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, -1.0f,
	};

	void Draw();

private:
	ScreenRectangle();
	~ScreenRectangle();

	GLuint vao_;
	GLuint vbo_;
};

class TextureVisualizer :public Singleton<TextureVisualizer> {
public:
	friend Singleton<TextureVisualizer>;

	void VisualizeTexture(GLuint texture_id, float scale);

private:
	TextureVisualizer();

	std::unique_ptr<GLProgram> program_;
};

