#include "HDRBuffer.h"

#include <stdexcept>
#include <array>

#include <glm/gtc/type_ptr.hpp>

#include "Textures.h"
#include "Samplers.h"
#include "ScreenRectangle.h"
#include "PerformanceMarker.h"

HDRBuffer::HDRBuffer(int width, int height) {
	framebuffers_.Create();
	textures_.Create(GL_TEXTURE_2D);

	for (int i = 0; i < framebuffers_.size(); ++i) {
		glTextureStorage2D(textures_[i], 1, GL_RGBA16F, width, height);

		glNamedFramebufferTexture(framebuffers_[i], GL_COLOR_ATTACHMENT0, textures_[i], 0);

		GLenum attachments[]{ GL_COLOR_ATTACHMENT0 };
		constexpr GLsizei attachments_num = sizeof(attachments) / sizeof(attachments[0]);
		glNamedFramebufferDrawBuffers(framebuffers_[i], attachments_num, attachments);

		if (glCheckNamedFramebufferStatus(framebuffers_[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			throw std::runtime_error("Framebuffer Incomplete");
		}
	}
}

void HDRBuffer::DoPostProcess(GLuint target_framebuffer_id, const PostProcessParameters& params) {
	PERF_MARKER("PostProcess")
	PostProcessRenderer::Instance().Render(target_framebuffer_id, *this, params);
}

HDRBuffer::PostProcessRenderer::PostProcessRenderer() {
	extract_ = []() {
		auto extract_src = ReadWithPreprocessor("../shaders/Base/ExtractColor.frag");
		return GLProgram(kCommonVertexSrc, extract_src.c_str());
	};

	pass1_ = []() {
		auto pass1_src = ReadWithPreprocessor("../shaders/Base/BloomPass1.frag");
		return  GLProgram(kCommonVertexSrc, pass1_src.c_str());
	};

	for (size_t i = 0; i < std::size(pass2_); ++i) {
		for (const auto& dither_enable : { 0, 1 }) {
			pass2_[i][dither_enable] = [i, dither_enable]() {
				auto pass2_src = ReadWithPreprocessor("../shaders/Base/BloomPass2.frag");
				std::string conf = "#define TONE_MAPPING ";
				conf += std::to_string(i);
				conf += "\n";
				conf += "#define DITHER_ENABLE ";
				conf += std::to_string(dither_enable);
				auto fragment_src = Replace(pass2_src, "TAG_CONF", conf);
				return GLProgram(kCommonVertexSrc, fragment_src.c_str());
			};
		}
	}
}

void HDRBuffer::PostProcessRenderer::Render(GLuint target_framebuffer_id, const HDRBuffer& hdrbuffer, const PostProcessParameters& params) {
	GLBindTextures({ hdrbuffer.textures_[0],
					hdrbuffer.textures_[1],
					hdrbuffer.textures_[2],
					params.dither_color_enable ? Textures::Instance().blue_noise() : 0 });
	GLBindSamplers({ static_cast<GLuint>(0),
					Samplers::GetLinearNoMipmapClampToEdge(),
					Samplers::GetLinearNoMipmapClampToEdge(),
					static_cast<GLuint>(0) });

	glBindFramebuffer(GL_FRAMEBUFFER, hdrbuffer.framebuffers_[1]);
	glUseProgram(extract_.id());
	glUniform1f(0, params.bloom_min_luminance);
	glUniform1f(1, params.bloom_max_delta_luminance);
	ScreenRectangle::Instance().Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, hdrbuffer.framebuffers_[2]);
	glUseProgram(pass1_.id());
	glUniform1f(0, params.bloom_filter_width);
	ScreenRectangle::Instance().Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, target_framebuffer_id);
	glUseProgram(pass2_[static_cast<uint32_t>(params.tone_mapping)][params.dither_color_enable].id());
	glUniform1f(0, params.bloom_filter_width);
	glUniform1f(1, params.bloom_intensity);
	glUniform1f(2, params.exposure);
	ScreenRectangle::Instance().Draw();
}
