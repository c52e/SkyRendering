#pragma once

#include <cmath>
#include <memory>

#include <stb_image.h>

inline int GetMipmapLevels(int width, int height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

inline int GetMipmapLevels(int width, int height, int depth) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(std::max(width, height), depth)))) + 1;
}

struct StbDeleter {
    void operator()(void* p) {
        stbi_image_free(p);
    }
};

inline std::unique_ptr<stbi_us, StbDeleter> stbi_load_16_unique(char const* filename, int* x, int* y, int* comp, int req_comp) {
    return { stbi_load_16(filename, x, y, comp, req_comp), StbDeleter() };
}

inline std::unique_ptr<stbi_uc, StbDeleter> stbi_load_unique(char const* filename, int* x, int* y, int* comp, int req_comp) {
    return { stbi_load(filename, x, y, comp, req_comp), StbDeleter() };
}

