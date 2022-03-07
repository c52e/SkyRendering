#include <iostream>
#include <stdexcept>
#include <memory>
#include <array>
#include <sstream>

#include <magic_enum.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "GLWindow.h"
#include "GLProgram.h"
#include "Camera.h"
#include "Atmosphere.h"
#include "Samplers.h"
#include "GBuffer.h"
#include "Utils.h"
#include "ImGuiExt.h"
#include "ScreenRectangle.h"
#include "Mesh.h"
#include "ShadowMap.h"

// Run with Nvidia GPU on laptop
extern "C" {
    _declspec(dllexport) unsigned NvOptimusEnablement = 0x00000001;
}

class MeshObject {
public:
    MeshObject(const Mesh* mesh, const glm::mat4& model, bool cast_shadow)
        :mesh_(mesh), model_(model), cast_shadow_(cast_shadow) {}

    virtual ~MeshObject() {}

    virtual void RenderToGBuffer(const glm::mat4& view_projection) const = 0;

    virtual void RenderToShadowMap(const glm::mat4& light_view_projection) const {
        if (cast_shadow_) {
            ShadowMapRenderer::Instance().Setup(model_, light_view_projection);
            mesh_->Draw();
        }
    }

    const glm::mat4& model() { return model_; }
    void set_model(const glm::mat4& model) { model_ = model; }

protected:
    const Mesh* mesh_;
    glm::mat4 model_;
    bool cast_shadow_;
};

class MeshConstAlbedoObject : public MeshObject {
public:
    MeshConstAlbedoObject(const Mesh* mesh, const glm::mat4& model, bool cast_shadow, glm::vec3 albedo)
        : MeshObject(mesh, model, cast_shadow), albedo_(albedo) {}

    ~MeshConstAlbedoObject() {}

    void RenderToGBuffer(const glm::mat4& view_projection) const override {
        GBufferRenderer::Instance().Setup(model_, view_projection, albedo_);
        mesh_->Draw();
    }

private:
    glm::vec3 albedo_;
};

class MeshWithTextureObject : public MeshObject {
public:
    MeshWithTextureObject(const Mesh* mesh, const glm::mat4& model, bool cast_shadow, GLuint texture)
        : MeshObject(mesh, model, cast_shadow), texture_(texture) {}

    ~MeshWithTextureObject() {}

    void RenderToGBuffer(const glm::mat4& view_projection) const override {
        GBufferRenderer::Instance().SetupWithTexture(model_, view_projection, texture_);
        mesh_->Draw();
    }

private:
    GLuint texture_;
};

class Demo : public GLWindow {
public:
    Demo(const char* name, int width, int height)
        : GLWindow(name, width, height, false) {
        Init();
    }
    ~Demo() {
    }

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<GBuffer> gbuffer_;
    std::unique_ptr<GBuffer> gbuffer_vr_left_;
    std::unique_ptr<GBuffer> gbuffer_vr_right_;
    std::unique_ptr<ShadowMap> shadow_map_;
    std::unique_ptr<Mesh> pyramid_;
    std::unique_ptr<Mesh> sphere_;
    std::unique_ptr<Mesh> tyrannosaurus_rex_;
    std::unique_ptr<Mesh> wall_;
    std::unique_ptr<Mesh> fence_;
    std::unique_ptr<Mesh> cylinder_;
    std::unique_ptr<Atmosphere> atmosphere_;
    std::unique_ptr<AtmosphereRenderer> atmosphere_renderer_;

    bool anisotropy_enable = true;
    AtmosphereRenderInitParameters atmosphere_render_init_parameters_;

    AtmosphereParameters atmosphere_parameters_;
    AtmosphereRenderParameters atmosphere_render_parameters_;

    std::vector<std::unique_ptr<MeshObject>> mesh_objects_;
    MeshWithTextureObject* moon_;
    float moon_direction_theta = 70.0f;
    float moon_direction_phi = 150.0f;
    float moon_distance = 384401.f / 10.f;
    float moon_radius = 1737.f;

    bool vr_mode_ = false;

    glm::mat4 GetMoonModel() {
        auto model_moon = glm::identity<glm::mat4>();
        model_moon = glm::translate(model_moon, glm::vec3(0, -atmosphere_parameters_.bottom_radius, 0));
        model_moon = glm::rotate(model_moon, glm::radians(moon_direction_phi), glm::vec3(0, 1, 0));
        model_moon = glm::rotate(model_moon, glm::radians(moon_direction_theta), glm::vec3(0, 0, -1));
        model_moon = glm::translate(model_moon,
            glm::vec3(0, moon_distance + atmosphere_parameters_.bottom_radius, 0));
        model_moon = glm::scale(model_moon, glm::vec3(moon_radius));
        model_moon = glm::rotate(model_moon, glm::radians(90.f), glm::vec3(0, 0, 1));
        return model_moon;
    }

    void Init() {
        Samplers::Instance().SetAnisotropyEnable(anisotropy_enable);
        atmosphere_ = std::make_unique<Atmosphere>();
        atmosphere_renderer_ = std::make_unique<AtmosphereRenderer>(atmosphere_render_init_parameters_);
        
        auto [width, height] = GetWindowSize();
        auto aspect = static_cast<float>(width) / height;
        camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 1.0f, -1.0f), 180.0f, 0.0f, aspect);
        HandleReshapeEvent(width, height);

        pyramid_ = std::make_unique<Mesh>(CreatePyramid());
        sphere_ = std::make_unique<Mesh>(CreateSphere());
        tyrannosaurus_rex_ = std::make_unique<Mesh>(ReadMeshFile("../data/models/TyrannosaurusRex.mesh"));
        wall_ = std::make_unique<Mesh>(ReadMeshFile("../data/models/wall.mesh"));
        fence_ = std::make_unique<Mesh>(ReadMeshFile("../data/models/fence.mesh"));
        cylinder_ = std::make_unique<Mesh>(ReadMeshFile("../data/models/cylinder.mesh"));

        shadow_map_ = std::make_unique<ShadowMap>(1024, 1024);

        {
            moon_ = new MeshWithTextureObject(
                sphere_.get(), GetMoonModel(), false, Textures::Instance().moon_albedo_texture());
            mesh_objects_.emplace_back(moon_);
        }
        /*{
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(-2, 0, 0));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                pyramid_.get(), model, true, glm::vec3(0.72, 0.63, 0.36)));
        }
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(2, 0, 0));
            model = glm::scale(model, glm::vec3(1, 1e2, 1));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                pyramid_.get(), model, true, glm::vec3(0.72, 0.72, 0.0)));
        }*/
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(0, 1, 1));
            mesh_objects_.emplace_back(new MeshWithTextureObject(
                sphere_.get(), model, true, Textures::Instance().earth_texture()));
        }
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(0, 0, -1));
            model = glm::scale(model, glm::vec3(0.3e-3, 0.3e-3, 0.3e-3));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, -1, 0));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(-1, 0, 0));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                tyrannosaurus_rex_.get(), model, true, glm::vec3(0.42, 0.42, 0.42)));
        }
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(-3, 0, -1.5));
            model = glm::scale(model, glm::vec3(1e-1, 1e-1, 1e-1));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, -1, 0));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                wall_.get(), model, true, glm::vec3(0.42, 0.42, 0.42)));
        }
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(-2, -0.1, -2));
            model = glm::scale(model, glm::vec3(1e-2, 1e-2, 1e-2));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(-1, 0, 0));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                fence_.get(), model, true, glm::vec3(0.42, 0.42, 0.42)));
        }
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(-2.2, 0.8, -2.0));
            model = glm::scale(model, glm::vec3(1e-2, 1e-2, 1e-2));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, 0, 1));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                fence_.get(), model, true, glm::vec3(0.42, 0.42, 0.42)));
        }
        {
            auto model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(2, 0, 0));
            model = glm::scale(model, glm::vec3(1e-1, 1e1, 1e-1));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(-1, 0, 0));
            mesh_objects_.emplace_back(new MeshConstAlbedoObject(
                cylinder_.get(), model, true, glm::vec3(0.42, 0.42, 0.42)));
        }
    }

    virtual void HandleDisplayEvent() override {
        atmosphere_->UpdateLuts(atmosphere_parameters_);

#if 1
        Render();
#else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        TextureVisualizer::Instance().VisualizeTexture(atmosphere_->multiscattering_texture(), 50.f);
#endif
        ProcessInput();
    }

    void Render() {
        auto light_view_projection_matrix = ComputeLightMatrix(5.f,
            atmosphere_render_parameters_.sun_direction_theta,
            atmosphere_render_parameters_.sun_direction_phi,
            -4.f, 4.f, -4.f, 4.f, 0, 5e1f);
        auto moon_model = GetMoonModel();
        moon_->set_model(moon_model);
        atmosphere_render_parameters_.moon_position = glm::vec3(moon_model[3]);
        atmosphere_render_parameters_.moon_radius = moon_radius;

        auto [width, height] = GetWindowSize();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        shadow_map_->ClearBindViewport();
        for (const auto& mesh_object : mesh_objects_)
            mesh_object->RenderToShadowMap(light_view_projection_matrix);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        if (vr_mode_) {
            auto vpleft = camera_->ViewProjectionVRLeft();
            auto vpright = camera_->ViewProjectionVRRight();
            auto pos_left = camera_->PositionVRLeft();
            auto pos_right = camera_->PositionVRRight();

            glViewport(0, 0, width / 2, height);
            RenderGBuffer(*gbuffer_vr_left_, vpleft);
            glViewport(0, 0, width - width / 2, height);
            RenderGBuffer(*gbuffer_vr_right_, vpright);

            glViewport(0, 0, width / 2, height);
            RenderViewport(*gbuffer_vr_left_, vpleft, pos_left, light_view_projection_matrix);
            glViewport(width / 2, 0, width - width / 2, height);
            RenderViewport(*gbuffer_vr_right_, vpright, pos_right, light_view_projection_matrix);
        }
        else {
            Clear(*gbuffer_);
            glViewport(0, 0, width, height);
            RenderGBuffer(*gbuffer_, camera_->ViewProjection());
            RenderViewport(*gbuffer_, camera_->ViewProjection(), camera_->position(), light_view_projection_matrix);
        }
    }

    void RenderGBuffer(const GBuffer& gbuffer, glm::mat4 vp) {
        Clear(gbuffer);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.id());
        glCullFace(GL_BACK);
        for (const auto& mesh_object : mesh_objects_)
            mesh_object->RenderToGBuffer(vp);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
    }

    void RenderViewport(const GBuffer& gbuffer, glm::mat4 vp, glm::vec3 pos, glm::mat4 light_view_projection_matrix) {
        atmosphere_render_parameters_.view_projection = vp;
        atmosphere_render_parameters_.camera_position = pos;
        atmosphere_render_parameters_.light_view_projection = light_view_projection_matrix;
        atmosphere_render_parameters_.depth_stencil_texture = gbuffer.depth_stencil_texture();
        atmosphere_render_parameters_.normal_texture = gbuffer.normal_texture();
        atmosphere_render_parameters_.albedo_texture = gbuffer.albedo_texture();
        atmosphere_render_parameters_.shadow_map_texture = shadow_map_->depth_texture();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        atmosphere_renderer_->Render(*atmosphere_, atmosphere_render_parameters_);
    }

    float camera_speed_ = 1.f;
    double mouse_x_ = 0.f;
    double mouse_y_ = 0.f;

    void ProcessInput() {
        if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse)
            return;

        static float previous_frame_time_ = 0.0f;
        float frame_time = static_cast<float>(glfwGetTime());
        float dt = frame_time - previous_frame_time_;
        previous_frame_time_ = frame_time;

        float dForward = 0.0f;
        float dRight = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            dForward += camera_speed_ * dt;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            dForward -= camera_speed_ * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            dRight += camera_speed_ * dt;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            dRight -= camera_speed_ * dt;
        auto move_vector = camera_->front() * dForward + camera_->right() * dRight;
        auto new_position = camera_->position() + move_vector;
        auto radius = atmosphere_parameters_.bottom_radius;
        auto center = glm::vec3(0, -radius, 0);
        if (glm::length(new_position - center) < radius) {
            new_position = glm::normalize(new_position - center) * (radius + 0.001f) + center;
        }
        camera_->set_position(new_position);

        float rotate_speed = 90.0f;
        float dPitch = 0.0f;
        float dYaw = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            dPitch += rotate_speed * dt;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            dPitch -= rotate_speed * dt;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            dYaw += rotate_speed * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            dYaw -= rotate_speed * dt;

        static bool previoud_left_button_down = false;
        static double previous_mouse_x_ = 0;
        static double previous_mouse_y_ = 0;
        bool left_button_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        if (!previoud_left_button_down && left_button_down) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            previous_mouse_y_ = mouse_y_;
            previous_mouse_x_ = mouse_x_;
        }
        else if (previoud_left_button_down && !left_button_down) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        previoud_left_button_down = left_button_down;

        if (left_button_down) {
            float mouse_rotate_speed = 10.f;
            dPitch += mouse_rotate_speed * static_cast<float>(previous_mouse_y_ - mouse_y_) * dt;
            dYaw += mouse_rotate_speed * static_cast<float>(mouse_x_ - previous_mouse_x_) * dt;
            previous_mouse_y_ = mouse_y_;
            previous_mouse_x_ = mouse_x_;
        }

        camera_->Rotate(dPitch, dYaw);
    }

    void SliderFloat(const char* name, float* ptr, float min_v, float max_v) {
        ImGui::SliderFloat(name, ptr, min_v, max_v);
    }

    void SliderFloatLogarithmic(const char* name, float* ptr, float min_v, float max_v, const char* format = "%.3f") {
        ImGui::SliderFloat(name, ptr, min_v, max_v, format, ImGuiSliderFlags_Logarithmic);
    }

    void ColorEdit(const char* name, glm::vec3& color) {
        ImGui::ColorEdit3(name, glm::value_ptr(color));
    }

    bool draw_gui_enable_ = true;
    bool full_screen_ = false;
    virtual void HandleDrawGuiEvent() override {
        if (!draw_gui_enable_)
            return;
        ImGui::Begin("GUI");
        ImGui::Text("%.3f ms (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

#ifdef ATMOSPHERE_PROFILE
        std::stringstream ss;
        auto transmittance_time_elapsed = atmosphere_->TransmittanceTimeElapsedMS();
        auto multiscattering_time_elapsed = atmosphere_->MultiscatteringTimeElapsedMS();
        auto sky_view_time_elapsed = atmosphere_renderer_->SkyViewTimeElapsedMS();
        auto aerial_perspective_time_elapsed = atmosphere_renderer_->AerialPerspectiveTimeElapsedMS();
        auto render_time_elapsed = atmosphere_renderer_->RenderTimeElapsedMS();
        ss << "Transmittance Time Elapsed(ms): " << transmittance_time_elapsed;
        ss << "\nMultiscattering Time Elapsed(ms): " << multiscattering_time_elapsed;
        ss << "\nSky View Time Elapsed(ms): " << sky_view_time_elapsed;
        ss << "\nAerial Perspective Time Elapsed(ms): " << aerial_perspective_time_elapsed;
        ss << "\nRender Time Elapsed(ms): " << render_time_elapsed;
        ss << "\nSum(ms): " << transmittance_time_elapsed + multiscattering_time_elapsed +
            sky_view_time_elapsed + aerial_perspective_time_elapsed + render_time_elapsed;
        ImGui::Text(ss.str().c_str());
#endif

        static bool previous_full_screen = false;
        ImGui::Checkbox("Full Screen", &full_screen_);
        if (full_screen_ != previous_full_screen)
            SetFullScreen(full_screen_);
        previous_full_screen = full_screen_;

        ImGui::SameLine();
        if (ImGui::Button("Hide GUI"))
            draw_gui_enable_ = false;
        ImGui::SameLine();
        ImGui::Text("(Press space to reshow)");

        static bool previous_anisotropy_enable = anisotropy_enable;
        static auto previous_atmosphere_render_init_parameters = atmosphere_render_init_parameters_;
        ImGui::Checkbox("Anisotropy Filtering Enable", &anisotropy_enable);
        ImGui::Checkbox("Dither Color Enable", &atmosphere_render_init_parameters_.dither_color_enable);
        ImGui::SameLine();
        ImGui::Checkbox("Volumetric Light Enable", &atmosphere_render_init_parameters_.volumetric_light_enable);
        ImGui::SameLine();
        ImGui::Checkbox("Moon Shadow Enable", &atmosphere_render_init_parameters_.moon_shadow_enable);
        EnumSelectable("Tone Mapping", &atmosphere_render_init_parameters_.tone_mapping);
        SliderFloat("Exposure", &atmosphere_render_parameters_.exposure, 0, 100.0);
        SliderFloatLogarithmic("Star Luminance Scale", &atmosphere_render_parameters_.star_luminance_scale, 0, 1.0);
        
        ImGui::Checkbox("VR Mode", &vr_mode_);
        static float zNear = 3e-1f;
        static float zFar = 5e4f;
        static float fovy = 45.0f;
        static float eye_distance = 1e-1f;
        SliderFloatLogarithmic("zNear", &zNear, 1e-1f, 1e2f, "%.1f");
        SliderFloatLogarithmic("zFear", &zFar, 1e2f, 1e6f, "%.1f");
        SliderFloatLogarithmic("fovy", &fovy, 0.0f, 180.0, "%.1f");
        SliderFloat("eye_distance", &eye_distance, 0.0f, 5e-1f);
        camera_->set_z_near(zNear);
        camera_->set_z_far(zFar);
        camera_->set_fovy(fovy);
        camera_->set_eye_distance(eye_distance);
        SliderFloatLogarithmic("Move Speed", &camera_speed_, 0.f, 1e5f, "%.1f");

        if (ImGui::TreeNode("Performance")) {
            SliderFloat("Transmittance Steps", &atmosphere_parameters_.transmittance_steps, 0, 100.0);
            SliderFloat("Multiscattering Steps", &atmosphere_parameters_.multiscattering_steps, 0, 100.0);
            SliderFloat("Multiscattering Mask", &atmosphere_parameters_.multiscattering_mask, 0, 1.0);
            if (!(atmosphere_render_init_parameters_.use_sky_view_lut && atmosphere_render_init_parameters_.use_aerial_perspective_lut)) {
                ImGui::Checkbox("Raymarching Dither Sample Point Enable", &atmosphere_render_init_parameters_.raymarching_dither_sample_point_enable);
                SliderFloat("Raymarching Steps", &atmosphere_render_parameters_.raymarching_steps, 0, 100.0);
            }
            ImGui::Checkbox("Use Sky View LUT", &atmosphere_render_init_parameters_.use_sky_view_lut);
            if (atmosphere_render_init_parameters_.use_sky_view_lut) {
                ImGui::Checkbox("Sky View LUT Dither Sample Point Enable", &atmosphere_render_init_parameters_.sky_view_lut_dither_sample_point_enable);
                SliderFloat("Sky View LUT Steps", &atmosphere_render_parameters_.sky_view_lut_steps, 0, 100.0);
            }
            ImGui::Checkbox("Use Aerial Perspective LUT", &atmosphere_render_init_parameters_.use_aerial_perspective_lut);
            if (atmosphere_render_init_parameters_.use_aerial_perspective_lut) {
                ImGui::Checkbox("Aerial Perspective LUT Dither Sample Point Enable", &atmosphere_render_init_parameters_.aerial_perspective_lut_dither_sample_point_enable);
                SliderFloat("Aerial Perspective LUT Steps", &atmosphere_render_parameters_.aerial_perspective_lut_steps, 0, 100.0);
                SliderFloat("Aerial Perspective LUT Max Distance", &atmosphere_render_parameters_.aerial_perspective_lut_max_distance, 0, 100.0);
            }
            ImGui::TreePop();
        }

        if (anisotropy_enable != previous_anisotropy_enable)
            Samplers::Instance().SetAnisotropyEnable(anisotropy_enable);
        previous_anisotropy_enable = anisotropy_enable;

        if (atmosphere_render_init_parameters_ != previous_atmosphere_render_init_parameters)
            atmosphere_renderer_ = std::make_unique<AtmosphereRenderer>(atmosphere_render_init_parameters_);
        previous_atmosphere_render_init_parameters = atmosphere_render_init_parameters_;

        if (ImGui::TreeNode("Sun")) {
            ColorEdit("Solar Illuminance", atmosphere_parameters_.solar_illuminance);
            SliderFloat("Sun Angular Radius", &atmosphere_parameters_.sun_angular_radius, 0, 30);
            SliderFloat("Sun Direction Theta", &atmosphere_render_parameters_.sun_direction_theta, 0, 180);
            SliderFloat("Sun Direction Phi", &atmosphere_render_parameters_.sun_direction_phi, 0, 360);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Moon")) {
            SliderFloat("Moon Direction Theta", &moon_direction_theta, 0, 180);
            SliderFloat("Moon Direction Phi", &moon_direction_phi, 0, 360);
            SliderFloatLogarithmic("Moon Distance From Earth", &moon_distance, 0, 1e6f, "%.0f");
            SliderFloatLogarithmic("Moon Radius", &moon_radius, 0, 1e4f, "%.0f");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Earth")) {
            ColorEdit("Ground Albedo", atmosphere_parameters_.ground_albedo);
            SliderFloatLogarithmic("Bottom Radius", &atmosphere_parameters_.bottom_radius, 0, 6360.0, "%.1f");
            SliderFloatLogarithmic("Thickness", &atmosphere_parameters_.thickness, 0, 1000.0, "%.1f");

            if (ImGui::TreeNode("Rayleigh")) {
                SliderFloatLogarithmic("Rayleigh Exponential Distribution", &atmosphere_parameters_.rayleigh_exponential_distribution, 0, 1000.0, "%.1f");
                SliderFloatLogarithmic("Rayleigh Scattering Scale", &atmosphere_parameters_.rayleigh_scattering_scale, 0, 1.0f, "%.4f");
                ColorEdit("Rayleigh Scattering", atmosphere_parameters_.rayleigh_scattering);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Mie")) {
                SliderFloatLogarithmic("Mie Exponential Distribution", &atmosphere_parameters_.mie_exponential_distribution, 0, 1000.0, "%.1f");
                SliderFloat("Mie Phase G", &atmosphere_parameters_.mie_phase_g, -1.0, 1.0);
                SliderFloatLogarithmic("Mie Scattering Scale", &atmosphere_parameters_.mie_scattering_scale, 0, 1.0f, "%.4f");
                ColorEdit("Mie Scattering", atmosphere_parameters_.mie_scattering);
                SliderFloatLogarithmic("Mie Absorption Scale", &atmosphere_parameters_.mie_absorption_scale, 0, 1.0f, "%.4f");
                ColorEdit("Mie Absorption", atmosphere_parameters_.mie_absorption);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Ozone")) {
                SliderFloatLogarithmic("Ozone Center Altitude", &atmosphere_parameters_.ozone_center_altitude, 0, 1000.0, "%.1f");
                SliderFloatLogarithmic("Ozone Width", &atmosphere_parameters_.ozone_width, 0, 1000.0, "%.1f");
                SliderFloatLogarithmic("Ozone Absorption Scale", &atmosphere_parameters_.ozone_absorption_scale, 0, 1.0f, "%.4f");
                ColorEdit("Ozone Absorption", atmosphere_parameters_.ozone_absorption);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        ImGui::End();

        ImGui::Begin("Textures");
        ImGui::Image((void*)(intptr_t)atmosphere_->transmittance_texture(), ImVec2(512, 512));
        ImGui::Image((void*)(intptr_t)atmosphere_->multiscattering_texture(), ImVec2(512, 512));
        ImGui::Image((void*)(intptr_t)gbuffer_->albedo_texture(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)gbuffer_->normal_texture(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)gbuffer_->depth_stencil_texture(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)shadow_map_->depth_texture(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
    }

    virtual void HandleReshapeEvent(int viewport_width, int viewport_height) override {
        gbuffer_ = std::make_unique<GBuffer>(viewport_width, viewport_height);
        gbuffer_vr_left_ = std::make_unique<GBuffer>(viewport_width / 2, viewport_height);
        gbuffer_vr_right_ = std::make_unique<GBuffer>(viewport_width - viewport_width / 2, viewport_height);
        camera_->set_aspect(static_cast<float>(viewport_width) / viewport_height);
    }

    virtual void HandleKeyboardEvent(int key) override {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            Close();
            break;
        case GLFW_KEY_SPACE:
            draw_gui_enable_ = !draw_gui_enable_;
            break;
        default:
            ;
        }
    }

    virtual void HandleMouseEvent(double mouse_x, double mouse_y) override {
        mouse_x_ = mouse_x;
        mouse_y_ = mouse_y;
    }

    virtual void HandleMouseWheelEvent(double yoffset) override {
    }

};

int main() {
    try {
        Demo demo("AtmosphereHillaire", 1280, 720);
        demo.MainLoop();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
