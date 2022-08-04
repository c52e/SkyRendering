#pragma once

#include <glm/glm.hpp>
#include <magic_enum.hpp>

#include "gl.hpp"
#include "Singleton.h"
#include "GLReloadableProgram.h"
#include "Serialization.h"

enum class ToneMapping {
	CEToneMapping,
	ACESToneMapping
};

struct PostProcessParameters : public ISerializable {
	ToneMapping tone_mapping = ToneMapping::ACESToneMapping;
	float bloom_min_luminance = 1.0f;
	float bloom_max_delta_luminance = 20.0f;
	float bloom_filter_width = 0.03f;
	float bloom_intensity = 0.1f;
	float exposure = 10.f;
	bool dither_color_enable = true;

	FIELD_DECLARATION_BEGIN(ISerializable)
		FIELD_DECLARE(tone_mapping)
		FIELD_DECLARE(bloom_min_luminance)
		FIELD_DECLARE(bloom_max_delta_luminance)
		FIELD_DECLARE(bloom_filter_width)
		FIELD_DECLARE(bloom_intensity)
		FIELD_DECLARE(exposure)
		FIELD_DECLARE(dither_color_enable)
	FIELD_DECLARATION_END()
};

class HDRBuffer {
public:
	HDRBuffer(int width, int height);

	GLuint hdr_texture() const { return hdr_textures_[0]; }
	GLuint sdr_texture() const { return sdr_texture_.id(); }

	void BindHdrFramebuffer() const;

	void BindSdrFramebuffer() const;
	
	void DoPostProcessAndBindSdrFramebuffer(const PostProcessParameters& params);

private:
	GLFramebuffer framebuffer_;
	GLTextures<3> hdr_textures_;
	GLTexture sdr_texture_;

	class PostProcessRenderer : public Singleton<PostProcessRenderer> {
	public:
		friend Singleton<PostProcessRenderer>;

		void Render(const HDRBuffer& hdrbuffer, const PostProcessParameters& params);
	private:
		PostProcessRenderer();
		GLReloadableProgram extract_;
		GLReloadableProgram pass1_;
		GLReloadableProgram pass2_[magic_enum::enum_count<ToneMapping>()][2];
	};
};

