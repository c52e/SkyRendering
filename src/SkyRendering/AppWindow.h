#pragma once

#include <memory>

#include <glm/gtc/type_ptr.hpp>

#include "GLWindow.h"
#include "Camera.h"
#include "Earth.h"
#include "AtmosphereRenderer.h"
#include "VolumetricCloud.h"
#include "GBuffer.h"
#include "HDRBuffer.h"
#include "MeshObject.h"
#include "ShadowMap.h"
#include "Serialization.h"

class AppWindow : public GLWindow, public ISerializable {
public:
    AppWindow(const char* config_path, int width, int height);

private:
    virtual void HandleDisplayEvent() override;
    virtual void HandleDrawGuiEvent() override;
    virtual void HandleReshapeEvent(int viewport_width, int viewport_height) override;
    virtual void HandleKeyboardEvent(int key) override;
    virtual void HandleMouseEvent(double mouse_x, double mouse_y) override;

    void Init(const char* config_path);
    char config_path_[128];
    bool SaveConfig(const char* path);
    void ProcessInput();

    void Render();
    void RenderShadowMap(const glm::mat4& light_view_projection_matrix);
    void RenderGBuffer(const GBuffer& gbuffer, const glm::mat4& vp);
    void RenderViewport(const GBuffer& gbuffer, const glm::mat4& vp, glm::vec3 pos, const glm::mat4& light_view_projection_matrix);

    std::unique_ptr<GBuffer> gbuffer_;
    std::unique_ptr<HDRBuffer> hdrbuffer_;
    std::unique_ptr<ShadowMap> shadow_map_;
    std::unique_ptr<AtmosphereRenderer> atmosphere_renderer_;

    Earth earth_;
    Camera camera_;
    VolumetricCloud volumetric_cloud_;
    AtmosphereRenderInitParameters atmosphere_render_init_parameters_;
    AtmosphereRenderParameters atmosphere_render_parameters_;
    PostProcessParameters post_process_parameters_;

    std::vector<std::unique_ptr<MeshObject>> mesh_objects_;
    MeshObject* moon_;

    float camera_speed_ = 1.f;
    double mouse_x_ = 0.f;
    double mouse_y_ = 0.f;

    bool draw_gui_enable_ = true;
    bool draw_debug_textures_enable_ = false;
    bool full_screen_ = false;
    bool anisotropy_enable_ = true;
    bool vsync_enable_ = false;

    FIELD_DECLARATION_BEGIN(ISerializable)
        FIELD_DECLARE(earth_)
        FIELD_DECLARE(volumetric_cloud_)
        FIELD_DECLARE(camera_)
        FIELD_DECLARE(atmosphere_render_init_parameters_)
        FIELD_DECLARE(atmosphere_render_parameters_)
        FIELD_DECLARE(post_process_parameters_)

        FIELD_DECLARE(camera_speed_)
        FIELD_DECLARE(draw_gui_enable_)
        FIELD_DECLARE(draw_debug_textures_enable_)
        FIELD_DECLARE(full_screen_)
        FIELD_DECLARE(anisotropy_enable_)
        FIELD_DECLARE(vsync_enable_)
    FIELD_DECLARATION_END()
};
