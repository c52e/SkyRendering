#include "VolumetricCloud.h"

#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#include "ImGuiExt.h"
#include "Textures.h"
#include "VolumetricCloudDefaultMaterial.h"

struct VolumetricCloudCommonBufferData {
	glm::mat4 uInvMVP;
	glm::mat4 uReprojectMat;

	glm::mat4 uLightVP;
	glm::mat4 uInvLightVP;
	glm::mat4 uShadowMapReprojectMat;

	glm::vec3 uCameraPos;
	uint32_t uBaseShadingIndex;

	glm::vec2 uLinearDepthParam;
	float uBottomAltitude;
	float uTopAltitude;

	glm::vec3 uSunDirection;
	float uFrameID;

	float uInvShadowFroxelMaxDistance;
	float uAerialPerspectiveLutMaxDistance;
	float uShadowFroxelMaxDistance;
	float uEarthRadius;
};

struct VolumetricCloudBufferData {
	float uSunIlluminanceScale;
	float uMaxRaymarchDistance;
	float uMaxRaymarchSteps;
	float uMaxVisibleDistance;

	glm::vec3 uEnvColorScale;
	float uShadowSteps;

	float uSunMultiscatteringSigmaScale;
	float uEnvMultiscatteringSigmaScale;
	float uShadowDistance;
	float padding_;
};

static const glm::ivec2 kShadowMapResolution{ 512, 512 };

VolumetricCloud::VolumetricCloud() {
	material = std::make_unique<VolumetricCloudDefaultMaterial0>();

	common_buffer_.Create();
	glNamedBufferStorage(common_buffer_.id(), sizeof(VolumetricCloudCommonBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);
	buffer_.Create();
	glNamedBufferStorage(buffer_.id(), sizeof(VolumetricCloudBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

	checkerboard_gen_program_ = {
		"../shaders/SkyRendering/CheckerboardGen.comp",
		{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}},
		[](const std::string& src) { return std::string("#version 460\n") + src; }
	};

	index_gen_program_ = {
		"../shaders/SkyRendering/VolumetricCloudIndexGen.comp",
		{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}},
		[](const std::string& src) { return std::string("#version 460\n") + src; }
	};

	reconstruct_program_ = {
		"../shaders/SkyRendering/VolumetricCloudReconstruct.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
		[](const std::string& src) { return std::string("#version 460\n") + src; }
	};

	upscale_program_ = {
		"../shaders/SkyRendering/VolumetricCloudUpscale.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
		[](const std::string& src) { return std::string("#version 460\n") + src; }
	};

	for (int i = 0; i < 2; ++i) {
		auto tag = "VOLUMETRIC_CLOUD_SHADOW_MAP_BLUR_PASS" + std::to_string(i);
		shadow_map_blur_program_[i] = {
			"../shaders/SkyRendering/VolumetricCloudShadowMapBlur.comp",
			{{8, 8}, {16, 8}, {8, 4}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
			[tag](const std::string& src) { return std::string("#version 460\n") + Replace(src, "TAG_CONF", tag); },
			tag
		};
	}

	shadow_froxel_gen_program_ = {
		"../shaders/SkyRendering/VolumetricCloudShadowFroxel.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
		[](const std::string& src) { return std::string("#version 460\n") + src; }
	};

	for (auto& shadow_map: shadow_maps_) {
		shadow_map.Create(GL_TEXTURE_2D);
		glTextureStorage2D(shadow_map.id(), 1, GL_RG32F, kShadowMapResolution.x, kShadowMapResolution.y);
	}
	shadow_map_sampler_.Create();
	glSamplerParameteri(shadow_map_sampler_.id(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(shadow_map_sampler_.id(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(shadow_map_sampler_.id(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(shadow_map_sampler_.id(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float border [] { 1e10f, 1.0f, 0.0f, 0.0f };
	glSamplerParameterfv(shadow_map_sampler_.id(), GL_TEXTURE_BORDER_COLOR, border);
}

void VolumetricCloud::SetViewport(int width, int height) {
	viewport_ = { width, height };
	viewport_data_ = std::make_unique<ViewportData>(viewport_);
}

VolumetricCloud::ViewportData::ViewportData(glm::ivec2 viewport) {
	checkerboard_depth_.Create(GL_TEXTURE_2D);
	glTextureStorage2D(checkerboard_depth_.id(), 1, GL_R32F, viewport.x / 2, viewport.y / 2);
	index_linear_depth_.Create(GL_TEXTURE_2D);
	glTextureStorage2D(index_linear_depth_.id(), 1, GL_RG32F, viewport.x / 4, viewport.y / 4);
	render_texture_.Create(GL_TEXTURE_2D);
	glTextureStorage2D(render_texture_.id(), 1, GL_RGBA16F, viewport.x / 4, viewport.y / 4);
	cloud_distance_texture_.Create(GL_TEXTURE_2D);
	glTextureStorage2D(cloud_distance_texture_.id(), 1, GL_R32F, viewport.x / 4, viewport.y / 4);
	for (auto& tex : reconstruct_texture_) {
		tex.Create(GL_TEXTURE_2D);
		glTextureStorage2D(tex.id(), 1, GL_RGBA16F, viewport.x / 2, viewport.y / 2);
	}

	shadow_froxel.Create(GL_TEXTURE_3D);
	glTextureStorage3D(shadow_froxel.id(), 1, GL_R16, viewport.x / 12, viewport.y / 12, 128);
}

static glm::mat4 GetLightProjection(const Camera& camera, const glm::mat4& light_view, const glm::mat4& inv_model, float max_distance) {
	auto far_plane_center = camera.position() + camera.front() * max_distance;
	auto tanHalfFovy = glm::tan(glm::radians(camera.fovy) * 0.5f);

	auto up = tanHalfFovy * max_distance * camera.up();
	auto right = camera.aspect() * tanHalfFovy * max_distance * camera.right();
	glm::vec3 vertices_world [] = {
		camera.position(),
		far_plane_center + up + right,
		far_plane_center + up - right,
		far_plane_center - up + right,
		far_plane_center - up - right,
	};
	glm::vec2 min_xy(1e10f);
	glm::vec2 max_xy(-1e10f);
	for (const auto& vertex_world : vertices_world) {
		auto vertex_local = inv_model * glm::vec4(vertex_world, 1.0);
		auto vertex_light = light_view * vertex_local;
		for (int i = 0; i < 2; ++i) {
			min_xy[i] = glm::min(min_xy[i], vertex_light[i]);
			max_xy[i] = glm::max(max_xy[i], vertex_light[i]);
		}
	}
	auto padding = 0.5f * (max_xy - min_xy) / glm::vec2(kShadowMapResolution);
	min_xy -= padding;
	max_xy += padding;
	auto light_projection = glm::ortho(min_xy.x, max_xy.x, min_xy.y, max_xy.y, -1.0f, 1.0f);
	return light_projection;
}

void VolumetricCloud::Update(const Camera& camera, const Earth& earth, const AtmosphereRenderer& atmosphere_render) {
	if (!viewport_data_)
		throw std::runtime_error("Volumetric cloud viewport is undefined");
	if (!material)
		return;

	if (material.get() != preframe_material_) {
		render_program_ = {
			"../shaders/SkyRendering/VolumetricCloudRender.comp",
			{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {32, 8}, {32, 16}, {32, 32}},
			CreateShaderPostProcess()
		};
		shadow_map_gen_program_ = {
			"../shaders/SkyRendering/VolumetricCloudShadowMap.comp",
			{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
			CreateShaderPostProcess()
		};
		preframe_material_ = material.get();
	}

	auto earth_radius = earth.parameters.bottom_radius;
	auto earth_center = earth.center();
	auto camera_pos = camera.position();
	auto up = glm::normalize(camera_pos - earth_center);
	auto origin = earth_center + up * earth_radius;
	auto front = glm::normalize(glm::cross(up, glm::vec3(1, 0, 0)));
	auto right = glm::normalize(glm::cross(up, front));
	auto model = glm::mat4{
		{front, 0},
		{right, 0},
		{up, 0},
		{origin, 1}
	};
	auto inv_model = glm::inverse(model);
	auto mvp = camera.ViewProjection() * model;

	auto pre_camera_pos = camera_pos_;
	auto pre_mvp = mvp_;
	auto delta_world = camera_pos - pre_camera_pos;
	auto delta_local = glm::mat3(inv_model) * delta_world;

	glm::vec2 additional_delta{ 0, 0 };
	material->Update(viewport_, camera, offset_from_first_ + glm::dvec2(delta_local), additional_delta);
	delta_local += glm::vec3(additional_delta, 0);
	glm::mat4 delta_mat(1.0f);
	delta_mat[3][0] = delta_local[0];
	delta_mat[3][1] = delta_local[1];
	glm::mat4 additional_delta_only_mat(1.0f);
	additional_delta_only_mat[3][0] = additional_delta[0];
	additional_delta_only_mat[3][1] = additional_delta[1];

	auto local_camera_pos = glm::vec3(inv_model * glm::vec4(camera_pos, 1.0f));
	auto local_sun_direction = glm::normalize(glm::mat3(inv_model) * atmosphere_render.sun_direction());

	auto light_view_up = local_sun_direction.x == 0.0f && local_sun_direction.y == 0.0f ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1);
	auto light_view = glm::lookAt(local_camera_pos, local_camera_pos - local_sun_direction, light_view_up);
	auto light_projection = GetLightProjection(camera, light_view, inv_model, shadow_map_max_distance);
	auto light_vp = light_projection * light_view;
	auto inv_light_vp = glm::inverse(light_vp);
	auto pre_model = model_;
	auto pre_light_vp = light_vp_;

	auto aerial_perspective = atmosphere_render.aerial_perspective_lut();

	VolumetricCloudCommonBufferData common_buffer;
	common_buffer.uInvMVP = glm::inverse(mvp);
	common_buffer.uReprojectMat = pre_mvp * delta_mat;
	common_buffer.uShadowMapReprojectMat = pre_light_vp * (glm::inverse(pre_model) * model) * additional_delta_only_mat * inv_light_vp;
	common_buffer.uLightVP = light_vp;
	common_buffer.uInvLightVP = inv_light_vp;
	common_buffer.uCameraPos = local_camera_pos;
	common_buffer.uBaseShadingIndex = frame_id_ & 0x3;
	common_buffer.uLinearDepthParam = { 1.0f / camera.zNear, (camera.zFar - camera.zNear) / (camera.zFar * camera.zNear) };
	common_buffer.uEarthRadius = earth_radius;
	common_buffer.uSunDirection = local_sun_direction;
	common_buffer.uBottomAltitude = bottom_altitude_;
	common_buffer.uTopAltitude = bottom_altitude_ + thickness_;
	common_buffer.uFrameID = static_cast<float>(frame_id_);
	common_buffer.uShadowFroxelMaxDistance = shadow_froxel_max_distance;
	common_buffer.uAerialPerspectiveLutMaxDistance = aerial_perspective.max_distance;
	common_buffer.uInvShadowFroxelMaxDistance = GetShadowFroxel().inv_max_distance;

	glNamedBufferSubData(common_buffer_.id(), 0, sizeof(common_buffer), &common_buffer);

	atmosphere_transmittance_tex_ = earth.atmosphere().transmittance_texture();
	aerial_perspective_luminance_tex_ = aerial_perspective.luminance_tex;
	aerial_perspective_transmittance_tex_ = aerial_perspective.transmittance_tex;
	environment_luminance_tex_ = atmosphere_render.environment_luminance_texture();


	VolumetricCloudBufferData buffer;
	buffer.uMaxRaymarchDistance = max_raymarch_distance_;
	buffer.uMaxRaymarchSteps = max_raymarch_steps_;
	buffer.uMaxVisibleDistance = max_visible_distance_;
	buffer.uEnvColorScale = env_color_ * env_color_scale_;
	buffer.uSunIlluminanceScale = sun_illuminance_scale_;
	buffer.uShadowSteps = shadow_steps_;
	buffer.uShadowDistance = shadow_distance_;
	buffer.uSunMultiscatteringSigmaScale = sun_multiscattering_sigma_scale;
	buffer.uEnvMultiscatteringSigmaScale = env_multiscattering_sigma_scale;

	glNamedBufferSubData(buffer_.id(), 0, sizeof(buffer), &buffer);

	offset_from_first_ += glm::dvec2(delta_local);
	frame_id_ = (frame_id_ + 1) & 0xff;
	camera_pos_ = camera_pos;
	mvp_ = mvp;
	light_vp_inv_model_ = light_vp * inv_model;
	model_ = model;
	light_vp_ = light_vp;
}

void VolumetricCloud::RenderShadow() {
	PERF_MARKER("VolumetricCloudShadow");
	std::swap(shadow_maps_[0], shadow_maps_[1]);
	material->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, common_buffer_.id());
	{
		PERF_MARKER("VolumetricCloudShadowMap");
		GLBindTextures({ shadow_maps_[1].id(),
						Textures::Instance().blue_noise(), });
		GLBindSamplers({ shadow_map_sampler_.id(),
						0u});
		GLBindImageTextures({ shadow_maps_[0].id() });

		glUseProgram(shadow_map_gen_program_.id());
		shadow_map_gen_program_.Dispatch(kShadowMapResolution);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}
	{
		PERF_MARKER("VolumetricCloudShadowMapBlur");
		GLBindTextures({ shadow_maps_[0].id() });
		GLBindSamplers({ shadow_map_sampler_.id() });
		GLBindImageTextures({ shadow_maps_[1].id() });
		glUseProgram(shadow_map_blur_program_[0].id());
		shadow_map_blur_program_[0].Dispatch(kShadowMapResolution);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		GLBindTextures({ shadow_maps_[1].id() });
		GLBindSamplers({ shadow_map_sampler_.id() });
		GLBindImageTextures({ shadow_maps_[2].id() });
		glUseProgram(shadow_map_blur_program_[1].id());
		shadow_map_blur_program_[1].Dispatch(kShadowMapResolution);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}
	{
		PERF_MARKER("VolumetricCloudShadowFroxel");
		GLBindTextures({ shadow_maps_[2].id() });
		GLBindSamplers({ shadow_map_sampler_.id() });
		GLBindImageTextures({ viewport_data_->shadow_froxel.id() });

		glUseProgram(shadow_froxel_gen_program_.id());
		shadow_froxel_gen_program_.Dispatch(viewport_ / 12);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}
}

void VolumetricCloud::Render(GLuint hdr_texture, GLuint depth_texture) {
	if (path_tracing_) {
		path_tracing_->Render(hdr_texture);
		return;
	}

	PERF_MARKER("VolumetricCloud");
	{
		PERF_MARKER("Checkerboard Depth");
		GLBindTextures({ depth_texture });
		GLBindSamplers({ Samplers::GetNearestClampToEdge() });
		GLBindImageTextures({ viewport_data_->checkerboard_depth_.id() });

		glUseProgram(checkerboard_gen_program_.id());
		checkerboard_gen_program_.Dispatch(viewport_ / 2);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	material->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, common_buffer_.id());

	{
		PERF_MARKER("Index Generate");
		GLBindTextures({ viewport_data_->checkerboard_depth_.id() });
		GLBindSamplers({ Samplers::GetNearestClampToEdge() });
		GLBindImageTextures({ viewport_data_->index_linear_depth_.id() });

		glUseProgram(index_gen_program_.id());
		index_gen_program_.Dispatch(viewport_ / 4);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, 2, buffer_.id());

	{
		PERF_MARKER("Render");
		GLBindTextures<1>({ //viewport_data_->checkerboard_depth_.id(),
						viewport_data_->index_linear_depth_.id(),
						atmosphere_transmittance_tex_,
						aerial_perspective_luminance_tex_,
						aerial_perspective_transmittance_tex_,
						Textures::Instance().blue_noise(),
						GetShadowFroxel().shadow_froxel });

		GLBindSamplers<1>({ //Samplers::GetNearestClampToEdge(),
						Samplers::GetNearestClampToEdge(),
						Samplers::GetLinearNoMipmapClampToEdge(),
						Samplers::GetLinearNoMipmapClampToEdge(),
						Samplers::GetLinearNoMipmapClampToEdge(),
						0u,
						GetShadowFroxel().sampler, });

		GLBindImageTextures({ viewport_data_->render_texture_.id(),
							viewport_data_->cloud_distance_texture_.id() });

		glUseProgram(render_program_.id());
		render_program_.Dispatch(viewport_ / 4);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	{
		PERF_MARKER("Reconstruct");
		GLBindTextures<2>({// checkerboard_depth_.id(),
						// index_linear_depth_.id(),
						viewport_data_->render_texture_.id(),
						viewport_data_->cloud_distance_texture_.id(),
						viewport_data_->reconstruct_texture_[1].id(), });
		GLBindSamplers<2>({// Samplers::GetNearestClampToEdge(),
						// Samplers::GetNearestClampToEdge(),
						Samplers::GetNearestClampToEdge(),
						Samplers::GetNearestClampToEdge(),
						Samplers::GetLinearNoMipmapClampToEdge(), });
		GLBindImageTextures({ viewport_data_->reconstruct_texture_[0].id() });

		glUseProgram(reconstruct_program_.id());
		reconstruct_program_.Dispatch(viewport_ / 2);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	{
		PERF_MARKER("Upscale");
		GLBindTextures<1>({// checkerboard_depth_.id(),
						depth_texture,
						viewport_data_->reconstruct_texture_[0].id() });
		GLBindSamplers<1>({// Samplers::GetNearestClampToEdge(),
						Samplers::GetNearestClampToEdge(),
						Samplers::GetNearestClampToEdge() });
		GLBindImageTextures({ hdr_texture });

		glUseProgram(upscale_program_.id());
		upscale_program_.Dispatch(viewport_);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	using std::swap;
	swap(viewport_data_->reconstruct_texture_[0], viewport_data_->reconstruct_texture_[1]);
}

void VolumetricCloud::DrawGUI() {
	if (ImGui::Button(u8"StartPathTracing")) {
		path_tracing_ = std::make_unique<PathTracing>(*this, path_tracing_init_param_);
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"StopPathTracing")) {
		path_tracing_.reset();
	}
	if (ImGui::TreeNode("Path Tracing Settings")) {
		if (path_tracing_) {
			ImGui::Text("Frame Count: %u", path_tracing_->frame_cnt());
		}
		ImGui::SliderInt("Tile Count (Sqrt)", &path_tracing_init_param_.sqrt_tile_count, 1, 8);
		static int bounces_slider_max = 1024;
		ImGui::SliderInt("Bounces Slider Max", &bounces_slider_max, 32, 1024);
		ImGui::SliderInt("Max Bounces", &path_tracing_init_param_.max_bounces, 1, bounces_slider_max);
		ImGui::SliderFloat("Region Box Half Width", &path_tracing_init_param_.region_box_half_width, 0.0f, 200.0f);
		ImGui::SliderFloat("Forward Phase G", &path_tracing_init_param_.forward_phase_g, 0.001f, 1.0f);
		ImGui::SliderFloat("Back Phase G", &path_tracing_init_param_.back_phase_g, -1.0f, 0.001f);
		ImGui::SliderFloat("Forward Scattering Ratio", &path_tracing_init_param_.forward_scattering_ratio, 0.0f, 1.0f);
		ImGui::EnumSelect("PRNG", &path_tracing_init_param_.prng);
		ImGui::EnumSelect("Environment Lighting", &path_tracing_init_param_.environment_lighting);
		ImGui::Checkbox("Importance Sampling", &path_tracing_init_param_.importance_sampling);
		ImGui::TreePop();
	}
	ImGui::SliderFloat("Bottom Altitude", &bottom_altitude_, 0.0f, 10.0f);
	ImGui::SliderFloat("Thickness", &thickness_, 0.0f, 10.0f);
	ImGui::SliderFloat("Max Visible Distance", &max_visible_distance_, 0.0f, 1e5f, "%.0f", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("Max Raymarch Distance", &max_raymarch_distance_, 0.0f, 100.0f);
	ImGui::SliderFloat("Max Raymarch Steps", &max_raymarch_steps_, 0.0f, 256.0f);
	ImGui::ColorEdit3("Environment Color", glm::value_ptr(env_color_));
	ImGui::SliderFloat("Environment Color Scale", &env_color_scale_, 0.0f, 1.0f);
	ImGui::SliderFloat("Sun Illuminance Scale", &sun_illuminance_scale_, 0.0f, 1.0f);
	ImGui::SliderFloat("Shadow Steps", &shadow_steps_, 0.0f, 10.0f);
	ImGui::SliderFloat("Shadow Distance", &shadow_distance_, 0.0f, 10.0f);
	ImGui::SliderFloat("Shadow Map Max Distance", &shadow_map_max_distance, 0.0f, 100.0f);
	ImGui::SliderFloat("Shadow Froxel Max Distance", &shadow_froxel_max_distance, 0.0f, 100.0f);
	ImGui::SliderFloat("Sun Multiscattering Sigma Scale", &sun_multiscattering_sigma_scale, 0.0f, 5.0f);
	ImGui::SliderFloat("Environment Multiscattering Sigma Scale", &env_multiscattering_sigma_scale, 0.0f, 5.0f);
	auto b_material_tree_node = ImGui::TreeNode("Material");
	if (ImGui::BeginPopupContextItem()) {
		for (const auto& [name, factory] : reflection::SubclassInfo<IVolumetricCloudMaterial>::GetFactoryTable()) {
			if (ImGui::MenuItem(name.c_str())) {
				material.reset(factory());
				break;
			}
		}
		ImGui::EndPopup();
	}
	if (b_material_tree_node) {
		if (material)
			material->DrawGUI();
		ImGui::TreePop();
	}
}

std::function<std::string(const std::string&)> VolumetricCloud::CreateShaderPostProcess(std::string additional) const {
	return[material_path = material->ShaderPath(), additional = std::move(additional)](const std::string& src) {
		std::stringstream ss;
		ss << "#version 460\n";
		ss << additional << "\n";
		ss << "#define MATERIAL_TEXTURE_UNIT_BEGIN " << std::to_string(IVolumetricCloudMaterial::kMaterialTextureUnitBegin) << "\n";
		ss << src << "\n";
		ss << ReadWithPreprocessor(material_path.c_str());
		return ss.str();
	};
}

VolumetricCloud::PathTracing::PathTracing(const VolumetricCloud& cloud, const InitParam& init)
	: cloud_(cloud), sqrt_tile_count_(init.sqrt_tile_count) {
	const auto& viewport = cloud.viewport_;
	accumulating_texture_.Create(GL_TEXTURE_2D);
	glTextureStorage2D(accumulating_texture_.id(), 1, GL_RGBA32F, viewport.x, viewport.y);
	float zero[]{ 0,0,0,0 };
	glClearTexImage(accumulating_texture_.id(), 0, GL_RGBA, GL_FLOAT, zero);
	rendered_mask_.Create(GL_TEXTURE_2D);
	glTextureStorage2D(rendered_mask_.id(), 1, GL_R8UI, viewport.x, viewport.y);

	std::stringstream additional;
	additional << "#define kSigmaTMax " << cloud_.material->GetSigmaTMax() << "\n";
	additional << "#define kMaxBounces " << init.max_bounces << "\n";
	additional << "#define kCloudHalfWidth " << init.region_box_half_width << "\n";
	additional << "#define IMPORTANCE_SAMPLING " << (init.importance_sampling ? 1 : 0) << "\n";
	additional << "#define kForwardPhaseG " << init.forward_phase_g << "\n";
	additional << "#define kBackPhaseG " << init.back_phase_g << "\n";
	additional << "#define kForwardScatteringRatio " << init.forward_scattering_ratio << "\n";
	additional << "#define PRNG " << magic_enum::enum_name(init.prng) << "\n";
	additional << "#define ENVIRONMENT_LIGHT_" << magic_enum::enum_name(init.environment_lighting) << "\n";
	const auto& m = cloud_.model_;
	additional << "#define kModelMatrix3 mat3("
		<< cloud_.model_[0][0] << "," << cloud_.model_[0][1] << "," << cloud_.model_[0][2] << ","
		<< cloud_.model_[1][0] << "," << cloud_.model_[1][1] << "," << cloud_.model_[1][2] << ","
		<< cloud_.model_[2][0] << "," << cloud_.model_[2][1] << "," << cloud_.model_[2][2] << ")\n";
	render_program_ = {
		"../shaders/SkyRendering/VolumetricCloudPathTracing.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {32, 8}, {32, 16}, {32, 32}},
		cloud_.CreateShaderPostProcess(additional.str())
	};
	additional << "#define DISPLAY_PASS\n";
	display_program_ = {
		"../shaders/SkyRendering/VolumetricCloudPathTracing.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {32, 8}, {32, 16}, {32, 32}},
		cloud_.CreateShaderPostProcess(additional.str())
	};
}

void VolumetricCloud::PathTracing::Render(GLuint hdr_texture) {
	if (tile_index_ == 0) {
		++frame_cnt_;
		uint8_t zero = 0;
		glClearTexImage(rendered_mask_.id(), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero);
	}

	GLBindTextures({ cloud_.atmosphere_transmittance_tex_,
					cloud_.aerial_perspective_luminance_tex_,
					cloud_.aerial_perspective_transmittance_tex_,
					cloud_.GetShadowFroxel().shadow_froxel,
					cloud_.environment_luminance_tex_ });
	GLBindSamplers({ Samplers::GetLinearNoMipmapClampToEdge(),
					Samplers::GetLinearNoMipmapClampToEdge(),
					Samplers::GetLinearNoMipmapClampToEdge(),
					cloud_.GetShadowFroxel().sampler,
					Samplers::GetAnisotropySampler(Samplers::Wrap::CLAMP_TO_EDGE) });
	GLBindImageTextures({ accumulating_texture_.id(),
						rendered_mask_.id(),
						hdr_texture });
	cloud_.material->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, cloud_.common_buffer_.id());
	glUseProgram(render_program_.id());
	glUniform1ui(0, frame_cnt_);
	auto region = GetRenderRegion();
	glUniform4iv(1, 1, glm::value_ptr(region));
	render_program_.Dispatch({ region.z, region.w });
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

	glUseProgram(display_program_.id());
	glUniform1ui(0, frame_cnt_);
	display_program_.Dispatch(cloud_.viewport_);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

	++tile_index_;
	tile_index_ %= sqrt_tile_count_ * sqrt_tile_count_;
}

glm::ivec4 VolumetricCloud::PathTracing::GetRenderRegion() const {
	auto x = tile_index_ % sqrt_tile_count_;
	auto y = tile_index_ / sqrt_tile_count_;
	auto region_size_base = cloud_.viewport_ / sqrt_tile_count_;
	auto big_region_count = cloud_.viewport_ % sqrt_tile_count_;
	auto get_index = [region_size_base, big_region_count](glm::ivec2 group_index) {
		return glm::min(group_index, big_region_count) * (region_size_base + 1)
			+ glm::max(group_index - big_region_count, 0) * region_size_base;
	};
	return glm::ivec4(get_index({ x, y }), get_index({ x + 1,y + 1}));
}
