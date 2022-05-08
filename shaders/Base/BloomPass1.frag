#version 460
in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 1) uniform sampler2D extraced_tex;
layout(location = 0) uniform float filter_width;
#include"Blur.glsl"
void main() {
	FragColor = Blur(extraced_tex, vTexCoord.xy, vec2(0, filter_width));
}