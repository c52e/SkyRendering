#include "Samplers.h"

#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Samplers::Samplers() {
    glCreateSamplers(1, &linear_clamp_to_edge_);
    glSamplerParameteri(linear_clamp_to_edge_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(linear_clamp_to_edge_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(linear_clamp_to_edge_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(linear_clamp_to_edge_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(linear_clamp_to_edge_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glCreateSamplers(1, &linear_no_mipmap_clamp_to_edge_);
    glSamplerParameteri(linear_no_mipmap_clamp_to_edge_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(linear_no_mipmap_clamp_to_edge_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(linear_no_mipmap_clamp_to_edge_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(linear_no_mipmap_clamp_to_edge_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(linear_no_mipmap_clamp_to_edge_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glCreateSamplers(1, &shadow_map_sampler_);
    glSamplerParameteri(shadow_map_sampler_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(shadow_map_sampler_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(shadow_map_sampler_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(shadow_map_sampler_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glSamplerParameterfv(shadow_map_sampler_, GL_TEXTURE_BORDER_COLOR, border_color);
    glSamplerParameteri(shadow_map_sampler_, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(shadow_map_sampler_, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

}

Samplers::~Samplers() {
    glDeleteSamplers(1, &linear_clamp_to_edge_);
    glDeleteSamplers(1, &linear_no_mipmap_clamp_to_edge_);
    glDeleteSamplers(1, &shadow_map_sampler_);
}

void Samplers::SetAnisotropyEnable(bool enable) {
    if (enable) {
        float max_texture_max_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_texture_max_anisotropy);
        glSamplerParameterf(linear_clamp_to_edge_, GL_TEXTURE_MAX_ANISOTROPY, max_texture_max_anisotropy);
    }
    else {
        glSamplerParameterf(linear_clamp_to_edge_, GL_TEXTURE_MAX_ANISOTROPY, 1.0f);
    }
}

static int GetMipmapLevels(int width, int height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

Textures::Textures() {
    int x, y, n;
    auto data = stbi_load_16("../data/BlueNoise/64_64/HDR_L_0.png", &x, &y, &n, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &blue_noise_texture_);
    glTextureStorage2D(blue_noise_texture_, 1, GL_R16, x, y);
    glTextureSubImage2D(blue_noise_texture_, 0, 0, 0, x, y, GL_RED, GL_UNSIGNED_SHORT, data);
    stbi_image_free(data);

    auto data2 = stbi_load("../data/NASA/lroc_color_poles_2048x2048.jpg", &x, &y, &n, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &moon_albedo_texture_);
    glTextureStorage2D(moon_albedo_texture_, GetMipmapLevels(x, y), GL_RGB8, x, y);
    glTextureSubImage2D(moon_albedo_texture_, 0, 0, 0, x, y, GL_RGB, GL_UNSIGNED_BYTE, data2);
    glGenerateTextureMipmap(moon_albedo_texture_);
    stbi_image_free(data2);

    auto data3 = stbi_load("../data/NASA/starmap_2020_4k.jpg", &x, &y, &n, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &star_texture_);
    glTextureStorage2D(star_texture_, 1, GL_RGB8, x, y); // 星空不用mipmap，从而实现闪烁的效果
    glTextureSubImage2D(star_texture_, 0, 0, 0, x, y, GL_RGB, GL_UNSIGNED_BYTE, data3);
    stbi_image_free(data3);

    auto data4 = stbi_load("../data/NASA/world.topo.bathy.200401.3x5400x2700.jpg", &x, &y, &n, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &earth_texture_);
    glTextureStorage2D(earth_texture_, GetMipmapLevels(x, y), GL_RGB8, x, y);
    glTextureSubImage2D(earth_texture_, 0, 0, 0, x, y, GL_RGB, GL_UNSIGNED_BYTE, data4);
    stbi_image_free(data4);
    glGenerateTextureMipmap(earth_texture_);
}

Textures::~Textures() {
    glDeleteTextures(1, &blue_noise_texture_);
    glDeleteTextures(1, &moon_albedo_texture_);
    glDeleteTextures(1, &star_texture_);
    glDeleteTextures(1, &earth_texture_);
}
