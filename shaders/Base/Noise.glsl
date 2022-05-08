uint WangHash(uint seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

const vec3 kPerlinGradients[16] = {
    {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
    {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
    {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1},
    {1, 1, 0}, {-1, 1, 0}, {0, -1, 1}, {0, -1, -1}
};

vec3 GetPerlinGradients(uint i, uint j, uint k, uint seed) {
    return kPerlinGradients[WangHash(seed + WangHash(i + WangHash(j + WangHash(k)))) & 0xf];
}
    
float PerlinNoise(vec3 p, uint freq, uint seed) {
    p *= float(freq);
    uvec3 ijk0 = uvec3(floor(p)) % freq;
    uvec3 ijk1 = uvec3(ceil(p)) % freq;
    uint i0 = ijk0.x;
    uint j0 = ijk0.y;
    uint k0 = ijk0.z;
    uint i1 = ijk1.x;
    uint j1 = ijk1.y;
    uint k1 = ijk1.z;

    vec3 t = p - floor(p);
    vec3 uvw = t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    float u = uvw.x;
    float v = uvw.y;
    float w = uvw.z;

    float x0 = t.x;
    float y0 = t.y;
    float z0 = t.z;
    float x1 = t.x - 1.0;
    float y1 = t.y - 1.0;
    float z1 = t.z - 1.0;

    return mix(mix(mix(dot(GetPerlinGradients(i0, j0, k0, seed), vec3(x0, y0, z0)),
                       dot(GetPerlinGradients(i1, j0, k0, seed), vec3(x1, y0, z0)), u),
                   mix(dot(GetPerlinGradients(i0, j1, k0, seed), vec3(x0, y1, z0)),
                       dot(GetPerlinGradients(i1, j1, k0, seed), vec3(x1, y1, z0)), u),v),
               mix(mix(dot(GetPerlinGradients(i0, j0, k1, seed), vec3(x0, y0, z1)),
                       dot(GetPerlinGradients(i1, j0, k1, seed), vec3(x1, y0, z1)), u),
                   mix(dot(GetPerlinGradients(i0, j1, k1, seed), vec3(x0, y1, z1)),
                       dot(GetPerlinGradients(i1, j1, k1, seed), vec3(x1, y1, z1)), u), v), w);
}

float WorleyNoise(vec2 p, uint freq, uint seed) {
    p *= float(freq);
    uvec2 ij = uvec2(floor(p));
    p += float(freq);
    float min_dist = 1e10;
    for (uint di = freq - 1; di <= freq + 1; ++di) {
        for (uint dj = freq - 1; dj <= freq + 1; ++dj) {
            uvec2 grid = ij + uvec2(di, dj);
            uvec2 gridseed = grid % freq;
            uint rnd0 = WangHash(seed + WangHash(gridseed.x + WangHash(gridseed.y)));
            uint rnd1 = WangHash(seed + WangHash(gridseed.x + WangHash(gridseed.y + 1)));
            vec2 g = vec2(grid) + vec2(rnd0, rnd1) / 4294967296.0;
            float dist = distance(p, g);
            min_dist = min(min_dist, dist);
        }
    }
    return min_dist;
}

float WorleyNoise(vec3 p, uint freq, uint seed) {
    p *= float(freq);
    uvec3 ijk = uvec3(floor(p));
    p += float(freq);
    float min_dist = 1e10;
    for (uint di = freq - 1; di <= freq + 1; ++di) {
        for (uint dj = freq - 1; dj <= freq + 1; ++dj) {
            for (uint dk = freq - 1; dk <= freq + 1; ++dk) {
                uvec3 grid = ijk + uvec3(di, dj, dk);
                uvec3 gridseed = grid % freq;
                uint rnd0 = WangHash(seed + WangHash(gridseed.x + WangHash(gridseed.y + WangHash(gridseed.z))));
                uint rnd1 = WangHash(seed + WangHash(gridseed.x + WangHash(gridseed.y + WangHash(gridseed.z + 1))));
                uint rnd2 = WangHash(seed + WangHash(gridseed.x + WangHash(gridseed.y + WangHash(gridseed.z + 2))));
                vec3 g = vec3(grid) + vec3(rnd0, rnd1, rnd2) / 4294967296.0;
                float dist = distance(p, g);
                min_dist = min(min_dist, dist);
            }
        }
    }
    return min_dist;
}
