#ifndef _COMMON_GLSL
#define _COMMON_GLSL

#define PI 3.1415926535897932384626433832795
#define INV_PI (1.0 / PI)

vec3 ProjectiveMul(mat4 m, vec3 v) {
	vec4 xyzw = m * vec4(v, 1.0);
	return xyzw.xyz / xyzw.w;
}

// https://en.wikipedia.org/wiki/Cube_mapping
vec3 ConvertCubUvToDir(int index, vec2 uv) {
    // convert range 0 to 1 to -1 to 1
    float u = uv.x;
    float v = uv.y;
    float uc = 2.0 * u - 1.0;
    float vc = 2.0 * v - 1.0;
    vec3 dir = vec3(0);
    switch (index)
    {
    case 0: dir = vec3(1.0, vc, -uc); break;	// POSITIVE X
    case 1: dir = vec3(-1.0, vc, uc); break;	// NEGATIVE X
    case 2: dir = vec3(uc, 1.0, -vc); break;	// POSITIVE Y
    case 3: dir = vec3(uc, -1.0, vc); break;	// NEGATIVE Y
    case 4: dir = vec3(uc, vc, 1.0); break;	// POSITIVE Z
    case 5: dir = vec3(-uc, vc, -1.0); break;	// NEGATIVE Z
    }
    return normalize(dir);
}

#endif