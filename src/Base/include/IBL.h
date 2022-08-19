#pragma once

#include "gl.hpp"
#include "GLReloadableProgram.h"

class IBL {
public:
	static constexpr int kPrefilteredRadianceResolution = 128;
	static constexpr int kRoughnessCount = 5;
	static_assert((1 << (kRoughnessCount - 1)) <= kPrefilteredRadianceResolution);

	IBL();

	void Precompute(GLuint environment_radiance_texture);

	GLuint env_radiance_sh_buffer() const {
		return env_radiance_sh_buffer_.id();
	}

	GLuint prefiltered_radiance() const {
		return prefiltered_radiance_.id();
	}

private:
	GLBuffer env_radiance_sh_buffer_;
	GLReloadableProgram env_radiance_sh_program_;

	static constexpr GLenum kPrefilteredRadianceFormat = GL_RGBA16F;
	GLTexture prefiltered_radiance_;
	GLReloadableComputeProgram prefilter_radiance_program_;
};
