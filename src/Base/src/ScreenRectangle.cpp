#include "ScreenRectangle.h"

#include <array>

#include "Samplers.h"

static const char* kTextureVisualizerFragmentSrc = R"(
#version 460
in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D tex;
layout(location = 0) uniform float scale;
void main() {
	FragColor = texture(tex, vec2(vTexCoord.x, vTexCoord.y)) * scale;
}
)";

#define QUAD_METHOD 2
/*
Tex Hit Rate
            0       1       2
1280x720    85.49   85.50   85.58
1920x1080   92.06   92.06   92.11

Tex SOL
            0       1       2
1280x720    19.4    20.1    20.4
1920x1080   44.6    44.0    45.0
*/

#if QUAD_METHOD == 0
static const float vertices[][2] = {
    {-1.0f,  1.0f},
    {-1.0f, -1.0f},
    { 1.0f,  1.0f },
    { 1.0f, -1.0f},
};
#elif QUAD_METHOD == 1
static const float vertices[][2] = {
    {-1.0f, -1.0f},
    { 1.0f, -1.0f},
    {-1.0f,  1.0f},
    { 1.0f,  1.0f },
};
#elif QUAD_METHOD == 2
static const float vertices[][2] = {
    {-1.0f, -1.0f},
    { 3.0f, -1.0f},
    {-1.0f,  3.0f }
};
#endif

ScreenRectangle::ScreenRectangle() {
    vao_.Create();
    vbo_.Create();
    glNamedBufferStorage(vbo_.id(), sizeof(vertices), vertices, 0);
    glBindVertexArray(vao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, vbo_.id());
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
    glEnableVertexAttribArray(0);
}

void ScreenRectangle::Draw() {
    glBindVertexArray(vao_.id());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(std::size(vertices)));
}


TextureVisualizer::TextureVisualizer() {
    program_ = GLProgram(kCommonVertexSrc, kTextureVisualizerFragmentSrc);
}

void TextureVisualizer::VisualizeTexture(GLuint texture_id, float scale) {
    glUseProgram(program_.id());
    glUniform1f(0, scale);

    GLBindTextures({ texture_id });
    GLBindSamplers({ Samplers::GetAnisotropySampler(Samplers::Wrap::CLAMP_TO_EDGE) });

    ScreenRectangle::Instance().Draw();
}

