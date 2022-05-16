#ifndef _COMMON_GLSL
#define _COMMON_GLSL

#define PI 3.1415926535897932384626433832795
#define INV_PI (1.0 / PI)

vec3 ProjectiveMul(mat4 m, vec3 v) {
	vec4 xyzw = m * vec4(v, 1.0);
	return xyzw.xyz / xyzw.w;
}

#endif