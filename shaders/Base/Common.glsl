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

void CreateOrthonormalBasis(vec3 N, out vec3 t0, out vec3 t1) {
#if 0
    const vec3 up = vec3(0.0, 1.0, 0.0);
    const vec3 right = vec3(1.0, 0.0, 0.0);
    if (abs(dot(N, up)) > 0.01) {
        t0 = normalize(cross(N, right));
        t1 = normalize(cross(N, t0));
    }
    else {
        t0 = normalize(cross(N, up));
        t1 = normalize(cross(N, t0));
    }
#else
    // https://graphics.pixar.com/library/OrthonormalB/paper.pdf
    float s = (N.z >= 0.0 ? 1.0 : -1.0);
    float a = -1.0 / (s + N.z);
    float b = N.x * N.y * a;
    t0 = vec3(1.0 + s * N.x * N.x * a, s * b, -s * N.x);
    t1 = vec3(b, s + N.y * N.y * a, -N.y);
#endif
}

#endif