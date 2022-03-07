#include "ScreenRectangle.h"

#include <array>

#include "Samplers.h"

static const char* kTextureVisualizerVertexSrc = R"(
#version 460
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
void main() {
	vTexCoord = aPos * 0.5 + 0.5;
	gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* kTextureVisualizerFragmentSrc = R"(
#version 460
in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D tex;
layout(location = 0) uniform float scale;
void main() {
	FragColor = texture(tex, vec2(vTexCoord.x, 1.0 - vTexCoord.y)) * scale;
}
)";

ScreenRectangle::ScreenRectangle() {
	glCreateVertexArrays(1, &vao_);
    glCreateBuffers(1, &vbo_);
    glNamedBufferStorage(vbo_, sizeof(vertices), vertices, 0);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

ScreenRectangle::~ScreenRectangle() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void ScreenRectangle::Draw() {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


TextureVisualizer::TextureVisualizer() {
    program_ = std::make_unique<GLProgram>(
        kTextureVisualizerVertexSrc, kTextureVisualizerFragmentSrc);
}

void TextureVisualizer::VisualizeTexture(GLuint texture_id, float scale) {
    glUseProgram(program_->id());
    glUniform1f(0, scale);

    std::array textures = { texture_id };
    glBindTextures(0, static_cast<GLsizei>(textures.size()), textures.data());
    std::array samplers = { Samplers::Instance().linear_clamp_to_edge() };
    glBindSamplers(0, static_cast<GLsizei>(samplers.size()), samplers.data());

    ScreenRectangle::Instance().Draw();
}

