#include "AppWindow.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <magic_enum.hpp>
#include <imgui.h>
#include <rapidjson/error/en.h>

#include "Textures.h"
#include "Samplers.h"
#include "ImGuiExt.h"
#include "PerformanceMarker.h"
#include "ScreenRectangle.h"

AppWindow::AppWindow(const char* config_path, int width, int height)
    : GLWindow((std::string("SkyRendering (") + config_path + ")").c_str(), width, height, false) {
    auto aspect = static_cast<float>(width) / height;
    camera_ = Camera(glm::vec3(0.0f, 1.0f, -1.0f), 180.0f, 0.0f, aspect);
    camera_.zNear = 3e-1f;
    camera_.zFar = 5e4f;
    HandleReshapeEvent(width, height);
    shadow_map_ = std::make_unique<ShadowMap>(2048, 2048);

    Init(config_path);
}

void AppWindow::Init(const char* config_path) {
    std::ifstream fin(config_path);
    if (fin) {
        using namespace rapidjson;
        auto str = std::string(std::istreambuf_iterator<char>{fin}, {});
        Document d;
        if (d.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(str.c_str()).HasParseError()) {
            std::ostringstream str;
            str << "Failed to parse \"" << config_path << "\" (" << "offset " << d.GetErrorOffset() << "): " << GetParseError_En(d.GetParseError());
            throw std::runtime_error(str.str());
        }
        Deserialize(d);
        std::cout << "\"" << config_path <<  "\" loaded" << std::endl;
    }
    else {
        SaveConfig(config_path);
        std::cout << "\"" << config_path << "\" not found. Default config created" << std::endl;
    }
    strcpy_s(config_path_, config_path);

    SetFullScreen(full_screen_);
    Samplers::SetAnisotropyEnable(anisotropy_enable_);
    glfwSwapInterval(vsync_enable_ ? 1 : 0);
    atmosphere_renderer_ = std::make_unique<AtmosphereRenderer>(atmosphere_render_init_parameters_);

    mesh_objects_.clear();
    {
        auto object = std::make_unique<MeshObject>("Moon", Meshes::Instance().sphere(), earth_.moon_model(), false);
        object->material.albedo_texture = Textures::Instance().moon_albedo();
        object->material.normal_texture = Textures::Instance().moon_normal();
        moon_ = object.get();
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(0, 1, 1));
        auto object = std::make_unique<MeshObject>("Tiny Earth", Meshes::Instance().sphere(), model, true);
        object->material.albedo_texture = Textures::Instance().earth_albedo();
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(0, 1, 3));
        model = glm::scale(model, glm::vec3(0.4f));
        auto object = std::make_unique<MeshObject>("Tiny Moon", Meshes::Instance().sphere(), model, true);
        object->material.albedo_texture = Textures::Instance().moon_albedo();
        object->material.normal_texture = Textures::Instance().moon_normal();
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(0, 0, -1));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, -1, 0));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(-1, 0, 0));
        model = glm::scale(model, glm::vec3(0.3e-3, 0.3e-3, 0.3e-3));
        auto object = std::make_unique<MeshObject>("Tyrannosaurus", Meshes::Instance().tyrannosaurus_rex(), model, true);
        object->material.albedo_factor = { 0.42, 0.42, 0.42 };
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(-3, 0, -1.5));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, -1, 0));
        model = glm::scale(model, glm::vec3(1e-1, 1e-1, 1e-1));
        auto object = std::make_unique<MeshObject>("Wall", Meshes::Instance().wall(), model, true);
        object->material.albedo_factor = { 0.42, 0.42, 0.42 };
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(-2, -0.1, -2));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(-1, 0, 0));
        model = glm::scale(model, glm::vec3(1e-2, 1e-2, 1e-2));
        auto object = std::make_unique<MeshObject>("Fence", Meshes::Instance().fence(), model, true);
        object->material.albedo_factor = { 0.42, 0.42, 0.42 };
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(-2.2, 0.8, -2.0));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(1e-2, 1e-2, 1e-2));
        auto object = std::make_unique<MeshObject>("Fence2", Meshes::Instance().fence(), model, true);
        object->material.albedo_factor = { 0.42, 0.42, 0.42 };
        mesh_objects_.push_back(std::move(object));
    }
    {
        auto model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(2, 0, 0));
        model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(-1, 0, 0));
        model = glm::scale(model, glm::vec3(1e-1, 1e-1, 1e1));
        auto object = std::make_unique<MeshObject>("Cylinder", Meshes::Instance().cylinder(), model, true);
        object->material.albedo_factor = { 0.42, 0.42, 0.42 };
        mesh_objects_.push_back(std::move(object));
    }
}

bool AppWindow::SaveConfig(const char* path) {
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    Serialize(writer);
    std::ofstream fout(path);
    if (!fout)
        return false;
    fout << sb.GetString();
    return true;
}

void AppWindow::HandleDisplayEvent() {
    earth_.Update();
    volumetric_cloud_.Update(camera_, earth_, *atmosphere_renderer_);
    moon_->set_model(earth_.moon_model());
    Render();
    ProcessInput();
}

void AppWindow::Render() {
    PERF_MARKER("Render")
    constexpr float kShadowRegionHalfWidth = 4.0f;
    constexpr float kShadowRegionNear = 0.0f;
    constexpr float kShadowRegionFar = 5e1f;
    auto light_view_projection_matrix = ComputeLightMatrix(5.f,
        atmosphere_render_parameters_.sun_direction_theta,
        atmosphere_render_parameters_.sun_direction_phi,
        -kShadowRegionHalfWidth, kShadowRegionHalfWidth,
        -kShadowRegionHalfWidth, kShadowRegionHalfWidth,
        kShadowRegionNear, kShadowRegionFar);

    auto sun_angular_radius = glm::radians(earth_.parameters.sun_angular_radius);
    atmosphere_render_parameters_.pcss_size_k = sun_angular_radius * (kShadowRegionFar - kShadowRegionNear) / kShadowRegionHalfWidth;
    atmosphere_render_parameters_.blocker_kernel_size_k = 2.0f * atmosphere_render_parameters_.pcss_size_k;

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    RenderShadowMap(light_view_projection_matrix);
    volumetric_cloud_.RenderShadow();

    Clear(*gbuffer_);
    auto [width, height] = GetWindowSize();
    glViewport(0, 0, width, height);
    RenderGBuffer(*gbuffer_, camera_.ViewProjection());
    RenderViewport(*gbuffer_, camera_.ViewProjection(), camera_.position(), light_view_projection_matrix);
    {
        volumetric_cloud_.Render(hdrbuffer_->hdr_texture(), gbuffer_->depth_stencil());
    }

    hdrbuffer_->DoPostProcessAndBindSdrFramebuffer(post_process_parameters_);
    smaa_->DoSMAA(hdrbuffer_->sdr_texture());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    TextureVisualizer::Instance().VisualizeTexture(smaa_->output_tex());
}

void AppWindow::RenderShadowMap(const glm::mat4& light_view_projection_matrix) {
    PERF_MARKER("RenderShadowMap")
    glCullFace(GL_FRONT);
    shadow_map_->ClearBindViewport();
    for (const auto& mesh_object : mesh_objects_)
        mesh_object->RenderToShadowMap(light_view_projection_matrix);
    glDisable(GL_DEPTH_TEST);
}

void AppWindow::RenderGBuffer(const GBuffer& gbuffer, const glm::mat4& vp) {
    PERF_MARKER("RenderGBuffer")
    Clear(gbuffer);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.id());
    glCullFace(GL_BACK);
    for (const auto& mesh_object : mesh_objects_)
        mesh_object->RenderToGBuffer(vp);
    earth_.RenderToGBuffer(camera_, gbuffer.depth_stencil());
    glDisable(GL_DEPTH_TEST);
}

void AppWindow::RenderViewport(const GBuffer& gbuffer, const glm::mat4& vp, glm::vec3 pos, const glm::mat4& light_view_projection_matrix) {
    PERF_MARKER("RenderViewport")
    atmosphere_render_parameters_.view_projection = vp;
    atmosphere_render_parameters_.camera_position = pos;
    atmosphere_render_parameters_.light_view_projection = light_view_projection_matrix;
    atmosphere_render_parameters_.depth_stencil_texture = gbuffer.depth_stencil();
    atmosphere_render_parameters_.normal = gbuffer.normal();
    atmosphere_render_parameters_.albedo = gbuffer.albedo();
    atmosphere_render_parameters_.orm = gbuffer.orm();
    atmosphere_render_parameters_.shadow_map_texture = shadow_map_->depth_texture();

    hdrbuffer_->BindHdrFramebuffer();
    atmosphere_renderer_->Render(earth_, volumetric_cloud_, atmosphere_render_parameters_);
}

void AppWindow::ProcessInput() {
    if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse)
        return;

    static float previous_frame_time_ = 0.0f;
    float frame_time = static_cast<float>(glfwGetTime());
    float dt = frame_time - previous_frame_time_;
    previous_frame_time_ = frame_time;

    float dForward = 0.0f;
    float dRight = 0.0f;
    float dUp = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        dForward += camera_speed_ * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        dForward -= camera_speed_ * dt;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        dRight += camera_speed_ * dt;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        dRight -= camera_speed_ * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        dUp += camera_speed_ * dt;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        dUp -= camera_speed_ * dt;
    auto move_vector = camera_.front() * dForward + camera_.right() * dRight + kWorldUp * dUp;
    auto new_position = camera_.position() + move_vector;
    auto radius = earth_.parameters.bottom_radius + camera_.zNear;
    auto center = earth_.center();
    if (glm::length(new_position - center) < radius) {
        new_position = glm::normalize(new_position - center) * (radius + 0.001f) + center;
    }
    camera_.set_position(new_position);

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

    camera_.Rotate(dPitch, dYaw);
}

static void SliderFloat(const char* name, float* ptr, float min_v, float max_v) {
    ImGui::SliderFloat(name, ptr, min_v, max_v);
}

static void SliderFloatLogarithmic(const char* name, float* ptr, float min_v, float max_v, const char* format = "%.3f") {
    ImGui::SliderFloat(name, ptr, min_v, max_v, format, ImGuiSliderFlags_Logarithmic);
}

static void ColorEdit(const char* name, glm::vec3& color) {
    ImGui::ColorEdit3(name, glm::value_ptr(color));
}

void AppWindow::HandleDrawGuiEvent() {
    if (!draw_gui_enable_)
        return;
    ImGui::Begin("Main");
    ImGui::Text("%.3f ms (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    if (ImGui::Button("Reload Shader")) {
        try {
            GLReloadableProgram::ReloadAll();
        }
        catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
    ImGui::SameLine();
    {
        static std::vector<std::string> config_paths;
        static std::vector<const char*> config_paths_cstr;
        static int index = 0;
        if (ImGui::Button("Load State..")) {
            ImGui::OpenPopup("Load State");
            namespace fs = std::filesystem;
            config_paths.clear();
            config_paths_cstr.clear();
            for (auto const& entry : fs::recursive_directory_iterator(fs::current_path())) {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".json") {
                    config_paths.emplace_back(fs::relative(entry.path(), fs::current_path()).string());
                    if (config_paths.back() == config_path_)
                        index = static_cast<int>(config_paths.size()) - 1;
                }
            }
            for (const auto& path : config_paths)
                config_paths_cstr.emplace_back(path.c_str());
        }
        if (ImGui::BeginPopupModal("Load State", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!config_paths_cstr.empty()) {
                if (index >= static_cast<int>(config_paths_cstr.size()))
                    index = static_cast<int>(config_paths_cstr.size()) - 1;
                ImGui::ListBox("Configs", &index, config_paths_cstr.data(), static_cast<int>(config_paths_cstr.size()));
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    try {
                        Init(config_paths_cstr[index]);
                    }
                    catch (std::exception& e) {
                        Error(std::string("Load state failed: ") + e.what());
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Save State..")) {
        ImGui::OpenPopup("Save State");
    }
    if (ImGui::BeginPopupModal("Save State", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("path", config_path_, std::size(config_path_));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (!SaveConfig(config_path_))
                Error("Save failed");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("ScreenShot")) {
        time_t rawtime{};
        std::time(&rawtime);
        std::tm timeinfo{};
        localtime_s(&timeinfo, &rawtime);
        char buffer[84];
        std::strftime(buffer, std::size(buffer), "%Y-%m-%d %H-%M-%S.png", &timeinfo);
        ScreenShot(buffer);
    }

    if (ImGui::Button("Hide GUI"))
        draw_gui_enable_ = false;
    ImGui::SameLine();
    ImGui::Text("(Press space to reshow)");
    ImGui::SameLine();
    ImGui::Checkbox("Show Debug Textures", &draw_debug_textures_enable_);
    ImGui::SameLine();
    ImGui::Checkbox("Show Help", &draw_help_enable_);

    bool previous_full_screen = full_screen_;
    bool previous_anisotropy_enable = anisotropy_enable_;
    bool previous_vsync_enable = vsync_enable_;
    auto previous_atmosphere_render_init_parameters = atmosphere_render_init_parameters_;
    auto previous_smaa_option = smaa_option_;

    ImGui::Checkbox("Full Screen", &full_screen_);
    ImGui::SameLine();
    ImGui::Checkbox("VSync", &vsync_enable_);

    if (ImGui::TreeNode("Render")) {
        ImGui::Checkbox("PCSS", &atmosphere_render_init_parameters_.pcss_enable);
        ImGui::SameLine();
        ImGui::Checkbox("Volumetric Light", &atmosphere_render_init_parameters_.volumetric_light_enable);
        ImGui::SameLine();
        ImGui::Checkbox("Moon Shadow", &atmosphere_render_init_parameters_.moon_shadow_enable);
        ImGui::SameLine();
        ImGui::Checkbox("Anisotropy Filtering", &anisotropy_enable_);
        ImGui::EnumSelect("SMAA", &smaa_option_);
        ImGui::Separator();

        SliderFloat("Transmittance Steps", &earth_.parameters.transmittance_steps, 0, 100.0);
        SliderFloat("Multiscattering Steps", &earth_.parameters.multiscattering_steps, 0, 100.0);
        SliderFloat("Multiscattering Mask", &earth_.parameters.multiscattering_mask, 0, 1.0);
        ImGui::Separator();

        ImGui::Checkbox("Use Sky View LUT", &atmosphere_render_init_parameters_.use_sky_view_lut);
        ImGui::Checkbox("Sky View LUT Dither Sample Point Enable", &atmosphere_render_init_parameters_.sky_view_lut_dither_sample_point_enable);
        SliderFloat("Sky View LUT Steps", &atmosphere_render_parameters_.sky_view_lut_steps, 0, 100.0);
        ImGui::Separator();

        ImGui::Checkbox("Use Aerial Perspective LUT", &atmosphere_render_init_parameters_.use_aerial_perspective_lut);
        ImGui::Checkbox("Aerial Perspective LUT Dither Sample Point Enable", &atmosphere_render_init_parameters_.aerial_perspective_lut_dither_sample_point_enable);
        SliderFloat("Aerial Perspective LUT Steps", &atmosphere_render_parameters_.aerial_perspective_lut_steps, 0, 100.0);
        SliderFloatLogarithmic("Aerial Perspective LUT Max Distance", &atmosphere_render_parameters_.aerial_perspective_lut_max_distance, 0, 7000.0, "%.0f");
        ImGui::SliderInt("Aerial Perspective LUT Depth", &atmosphere_render_init_parameters_.aerial_perspective_lut_depth, 1, 512);
        ImGui::Separator();

        ImGui::Checkbox("Raymarching Dither Sample Point Enable", &atmosphere_render_init_parameters_.raymarching_dither_sample_point_enable);
        SliderFloat("Raymarching Steps", &atmosphere_render_parameters_.raymarching_steps, 0, 100.0);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Camera")) {
        SliderFloatLogarithmic("zNear", &camera_.zNear, 1e-1f, 1e2f, "%.1f");
        SliderFloatLogarithmic("zFear", &camera_.zFar, 1e2f, 1e6f, "%.1f");
        SliderFloatLogarithmic("fovy", &camera_.fovy, 0.0f, 180.0, "%.1f");
        SliderFloatLogarithmic("Move Speed", &camera_speed_, 0.f, 1e5f, "%.1f");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Sun & Moon & Star")) {
        ColorEdit("Solar Illuminance", earth_.parameters.solar_illuminance);
        SliderFloat("Sun Angular Radius", &earth_.parameters.sun_angular_radius, 0, 30);
        SliderFloat("Sun Direction Theta", &atmosphere_render_parameters_.sun_direction_theta, 0, 180);
        SliderFloat("Sun Direction Phi", &atmosphere_render_parameters_.sun_direction_phi, 0, 360);
        ImGui::Separator();

        SliderFloat("Moon Direction Theta", &earth_.moon_status.direction_theta, 0, 180);
        SliderFloat("Moon Direction Phi", &earth_.moon_status.direction_phi, 0, 360);
        SliderFloatLogarithmic("Moon Distance From Earth", &earth_.moon_status.distance, 0, 1e6f, "%.0f");
        SliderFloatLogarithmic("Moon Radius", &earth_.moon_status.radius, 0, 1e4f, "%.0f");
        ImGui::Separator();

        SliderFloatLogarithmic("Star Luminance Scale", &atmosphere_render_parameters_.star_luminance_scale, 0, 1.0);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Earth")) {
        ColorEdit("Ground Albedo", earth_.parameters.ground_albedo);
        SliderFloatLogarithmic("Bottom Radius", &earth_.parameters.bottom_radius, 0, 6360.0, "%.1f");
        SliderFloatLogarithmic("Thickness", &earth_.parameters.thickness, 0, 1000.0, "%.1f");
        ImGui::Separator();

        SliderFloatLogarithmic("Rayleigh Exponential Distribution", &earth_.parameters.rayleigh_exponential_distribution, 0, 1000.0, "%.1f");
        SliderFloatLogarithmic("Rayleigh Scattering Scale", &earth_.parameters.rayleigh_scattering_scale, 0, 1.0f, "%.4f");
        ColorEdit("Rayleigh Scattering", earth_.parameters.rayleigh_scattering);
        ImGui::Separator();

        SliderFloatLogarithmic("Mie Exponential Distribution", &earth_.parameters.mie_exponential_distribution, 0, 1000.0, "%.1f");
        SliderFloat("Mie Phase G", &earth_.parameters.mie_phase_g, -1.0, 1.0);
        SliderFloatLogarithmic("Mie Scattering Scale", &earth_.parameters.mie_scattering_scale, 0, 1.0f, "%.4f");
        ColorEdit("Mie Scattering", earth_.parameters.mie_scattering);
        SliderFloatLogarithmic("Mie Absorption Scale", &earth_.parameters.mie_absorption_scale, 0, 1.0f, "%.4f");
        ColorEdit("Mie Absorption", earth_.parameters.mie_absorption);
        ImGui::Separator();
        
        SliderFloatLogarithmic("Ozone Center Altitude", &earth_.parameters.ozone_center_altitude, 0, 1000.0, "%.1f");
        SliderFloatLogarithmic("Ozone Width", &earth_.parameters.ozone_width, 0, 1000.0, "%.1f");
        SliderFloatLogarithmic("Ozone Absorption Scale", &earth_.parameters.ozone_absorption_scale, 0, 1.0f, "%.4f");
        ColorEdit("Ozone Absorption", earth_.parameters.ozone_absorption);
        ImGui::Separator();

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Post Process")) {
        ImGui::EnumSelect("Tone Mapping", &post_process_parameters_.tone_mapping);
        ImGui::Checkbox("Dither Color", &post_process_parameters_.dither_color_enable);
        SliderFloat("Bloom Min Luminance", &post_process_parameters_.bloom_min_luminance, 0, 2.0f);
        SliderFloat("Bloom Max Clamp Luminance", &post_process_parameters_.bloom_max_delta_luminance, 0, 50.0f);
        SliderFloat("Bloom Filter Width", &post_process_parameters_.bloom_filter_width, 0, 0.1f);
        SliderFloat("Bloom Intensity", &post_process_parameters_.bloom_intensity, 0, 1.0f);
        SliderFloat("Exposure", &post_process_parameters_.exposure, 0, 100.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("VolumetricCloud")) {
        volumetric_cloud_.DrawGUI();
        ImGui::TreePop();
    }

    ImGui::End();

    if (draw_debug_textures_enable_) {
        ImGui::Begin("Textures");
        ImGui::Image((void*)(intptr_t)earth_.atmosphere().transmittance_texture(), ImVec2(512, 512));
        ImGui::Image((void*)(intptr_t)earth_.atmosphere().multiscattering_texture(), ImVec2(512, 512));
        ImGui::Image((void*)(intptr_t)gbuffer_->albedo(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)gbuffer_->normal(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)gbuffer_->orm(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)gbuffer_->depth_stencil(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)shadow_map_->depth_texture(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image((void*)(intptr_t)Textures::Instance().env_brdf_lut(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
    }

    ImGui::Begin("Models");
    for (const auto& object : mesh_objects_) {
        object->DrawGui();
    }
    ImGui::End();

    ImGui::Begin("Compute Program");
    GLReloadableComputeProgram::DrawGUIAll();
    ImGui::End();

    if (draw_help_enable_) {
        ImGui::Begin("Help");
        ImGui::BulletText("The main window may be hidden by others. You can drag them away to show the main window and change parameters.");
        ImGui::BulletText("WASDQE to move camera (You can change speed in main window).");
        ImGui::BulletText("Hold right mouse button to rotate camera.");
        ImGui::End();
    }

    if (full_screen_ != previous_full_screen)
        SetFullScreen(full_screen_);

    if (vsync_enable_ != previous_vsync_enable)
        glfwSwapInterval(vsync_enable_ ? 1 : 0);

    if (anisotropy_enable_ != previous_anisotropy_enable)
        Samplers::SetAnisotropyEnable(anisotropy_enable_);

    if (atmosphere_render_init_parameters_ != previous_atmosphere_render_init_parameters)
        atmosphere_renderer_ = std::make_unique<AtmosphereRenderer>(atmosphere_render_init_parameters_);

    if (smaa_option_ != previous_smaa_option) {
        auto [width, height] = GetWindowSize();
        smaa_ = std::make_unique<SMAA>(width, height, smaa_option_);
    }
}

void AppWindow::HandleReshapeEvent(int viewport_width, int viewport_height) {
    if (viewport_width > 0 && viewport_height > 0) {
        volumetric_cloud_.SetViewport(viewport_width, viewport_height);
        gbuffer_ = std::make_unique<GBuffer>(viewport_width, viewport_height);
        hdrbuffer_ = std::make_unique<HDRBuffer>(viewport_width, viewport_height);
        camera_.set_aspect(static_cast<float>(viewport_width) / viewport_height);
        smaa_ = std::make_unique<SMAA>(viewport_width, viewport_height, smaa_option_);
    }
}

void AppWindow::HandleKeyboardEvent(int key) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
        Close();
        break;
    case GLFW_KEY_SPACE:
        draw_gui_enable_ = !draw_gui_enable_;
        break;
    }
}

void AppWindow::HandleMouseEvent(double mouse_x, double mouse_y) {
    mouse_x_ = mouse_x;
    mouse_y_ = mouse_y;
}
