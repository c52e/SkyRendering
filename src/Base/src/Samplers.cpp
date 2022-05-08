#include "Samplers.h"

Samplers::Samplers() {
    const GLint kGLWrap[] = { GL_CLAMP_TO_EDGE, GL_REPEAT };
    const GLint kGLMag[] = { GL_NEAREST, GL_LINEAR };
    const GLint kGLMipmapMin[] = { GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
    const GLint kGLNomipmapMin[] = { GL_NEAREST, GL_LINEAR };

    float max_texture_max_anisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_texture_max_anisotropy);

    for (int i = 0; i < kWrapCount; ++i) {
        for (int j = 0; j < kMagCount; ++j) {
            for (int k = 0; k < kMipmapMinCount; ++k) {
                auto& s = mipmap_[i][j][k];
                s.Create();
                glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_S, kGLWrap[i]);
                glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_T, kGLWrap[i]);
                glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_R, kGLWrap[i]);
                glSamplerParameteri(s.id(), GL_TEXTURE_MAG_FILTER, kGLMag[j]);
                glSamplerParameteri(s.id(), GL_TEXTURE_MIN_FILTER, kGLMipmapMin[k]);
            }
            for (int k = 0; k < kNomipmapMinCount; ++k) {
                auto& s = nomipmap_[i][j][k];
                s.Create();
                glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_S, kGLWrap[i]);
                glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_T, kGLWrap[i]);
                glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_R, kGLWrap[i]);
                glSamplerParameteri(s.id(), GL_TEXTURE_MAG_FILTER, kGLMag[j]);
                glSamplerParameteri(s.id(), GL_TEXTURE_MIN_FILTER, kGLNomipmapMin[k]);
            }
        }
        auto& s = anisotropy_[i];
        s.Create();
        glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_S, kGLWrap[i]);
        glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_T, kGLWrap[i]);
        glSamplerParameteri(s.id(), GL_TEXTURE_WRAP_R, kGLWrap[i]);
        glSamplerParameteri(s.id(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(s.id(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameterf(s.id(), GL_TEXTURE_MAX_ANISOTROPY, max_texture_max_anisotropy);
    }

    shadow_map_.Create();
    glSamplerParameteri(shadow_map_.id(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(shadow_map_.id(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(shadow_map_.id(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(shadow_map_.id(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glSamplerParameterfv(shadow_map_.id(), GL_TEXTURE_BORDER_COLOR, border_color);
    glSamplerParameteri(shadow_map_.id(), GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(shadow_map_.id(), GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}
