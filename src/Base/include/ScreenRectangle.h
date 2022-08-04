#pragma once

#include "gl.hpp"
#include "Singleton.h"
#include "GLProgram.h"

class ScreenRectangle :public Singleton<ScreenRectangle> {
public:
	friend Singleton<ScreenRectangle>;

	void Draw();

private:
	ScreenRectangle();

	GLVertexArray vao_;
	GLBuffer vbo_;
};

class TextureVisualizer :public Singleton<TextureVisualizer> {
public:
	friend Singleton<TextureVisualizer>;

	void VisualizeTexture(GLuint texture_id, float scale = 1.0f);

private:
	TextureVisualizer();

	GLProgram program_;
};

