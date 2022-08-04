#define SMAA_GLSL_4
#define SMAA_PRESET_HIGH

#ifdef VERTEX

#define SMAA_INCLUDE_PS 0
#include "SMAA.hlsl"

layout(location = 0) in vec2 aPos;
out vec2 texcoord;
out vec2 pixcoord;
out vec4 offset[3];
void main() {
	texcoord = aPos * 0.5 + 0.5;
	SMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);
	gl_Position = vec4(aPos, 0.0, 1.0);
}

#endif


#ifdef FRAGMENT

#include "SMAA.hlsl"

layout(binding = 0) uniform sampler2D edgesTex;
layout(binding = 1) uniform sampler2D areaTex;
layout(binding = 2) uniform sampler2D searchTex;

in vec2 texcoord;
in vec2 pixcoord;
in vec4 offset[3];
layout(location = 0) out vec4 res;
void main() {
	res = SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edgesTex, areaTex, searchTex, vec4(0));
}

#endif
