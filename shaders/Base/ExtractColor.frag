#version 460
in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D tex;
layout(location = 0) uniform float min_luminance;
layout(location = 1) uniform float max_delta_luminance;

void main() {
	vec3 L = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
	float luminance = dot(L, vec3(0.2126, 0.7152, 0.0722));
	float new_luminance = clamp(luminance - min_luminance, 0, max_delta_luminance);
	L = L * (luminance == 0.0 ? 0 : new_luminance / luminance);
	FragColor = vec4(L, 1);
}
