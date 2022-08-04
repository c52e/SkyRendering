#pragma once

#include "gl.hpp"
#include "GLReloadableProgram.h"

enum class SMAAOption {
	OFF,
	SMAA_PRESET_LOW,
	SMAA_PRESET_MEDIUM,
	SMAA_PRESET_HIGH,
	SMAA_PRESET_ULTRA,
};

class SMAA {
public:
	SMAA(int width, int height, SMAAOption option);

	GLuint edges_tex() const { return edges_tex_.id(); }

	GLuint blend_tex() const { return blend_tex_.id(); }

	GLuint output_tex() const { return output_tex_.id(); }

	void DoSMAA(GLuint input); // Will change current framebuffer

private:
	void EdgesDetectionPass(GLuint input);

	void BlendingWeightsCalculationPass();

	void NeighborhoodBlendingPass(GLuint input);

	SMAAOption option_;

	GLReloadableProgram edge_detection_;
	GLReloadableProgram blending_weight_calculation_;
	GLReloadableProgram neighborhood_blending_;

	GLTexture area_tex_;
	GLTexture search_tex_;

	GLFramebuffer framebuffer_;
	GLTexture edges_tex_;
	GLTexture blend_tex_;
	GLTexture output_tex_;
	GLTexture stencil_tex_;
};
