#include "VolumetricCloud.h"

#include <sstream>

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "Textures.h"
#include "Samplers.h"
#include "VolumetricCloudDefaultMaterial.h"

struct VolumetricCloudCommonBufferData {
	glm::mat4 uInvMVP;
	glm::mat4 uPreMVP;
	glm::vec3 uCameraPos;
	uint32_t uBaseShadingIndex;
	glm::vec2 uFrameDelta;
	glm::vec2 uLinearDepthParam;
};

struct VolumetricCloudBufferData {
	glm::vec3 uSunDirection;
	float uSunIlluminanceScale;
	
	float uBottomAltitude;
	float uTopAltitude;
	float uMaxRaymarchDistance;
	float uMaxRaymarchSteps;

	glm::vec3 _cloud_padding;
	float uMaxVisibleDistance;

	glm::vec3 uEnvColorScale;
	float uShadowSteps;

	float uShadowDistance;
	float uAerialPerspectiveLutMaxDistance;
	float uFrameID;
	float uEarthRadius;
};

VolumetricCloud::VolumetricCloud() {
	material = std::make_unique<VolumetricCloudDefaultMaterial>();

	common_buffer_.Create();
	glNamedBufferStorage(common_buffer_.id(), sizeof(VolumetricCloudCommonBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);
	buffer_.Create();
	glNamedBufferStorage(buffer_.id(), sizeof(VolumetricCloudBufferData), NULL, GL_DYNAMIC_STORAGE_BIT);

	checkerboard_gen_program_ = {
		"../shaders/SkyRendering/CheckerboardGen.comp",
		{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}},
		[] { return std::string("#version 460\n"); }
	};

	index_gen_program_ = {
		"../shaders/SkyRendering/VolumetricCloudIndexGen.comp",
		{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}},
		[] { return std::string("#version 460\n"); }
	};

	reconstruct_program_ = {
		"../shaders/SkyRendering/VolumetricCloudReconstruct.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
		[] { return std::string("#version 460\n"); }
	};

	upscale_program_ = {
		"../shaders/SkyRendering/VolumetricCloudUpscale.comp",
		{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {16, 16}, {32, 8}, {32, 16}},
		[] { return std::string("#version 460\n"); }
	};
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
}

void VolumetricCloud::Render(GLuint hdr_texture, GLuint depth_texture, const Camera& camera
	, const Earth& earth, const AtmosphereRenderer& atmosphere_render) {
	if (!viewport_data_)
		throw std::runtime_error("Volumetric cloud viewport is undefined");
	if (!material)
		return;

	if (material.get() != preframe_material_) {
		render_program_ = {
			material->ShaderPath(),
			{{16, 8}, {8, 4}, {8, 8}, {16, 4}, {32, 8}, {32, 16}, {32, 32}},
			[] {
				std::stringstream ss;
				ss << "#version 460\n";
				ss << "#define MATERIAL_TEXTURE_UNIT_BEGIN " << std::to_string(IVolumetricCloudMaterial::kMaterialTextureUnitBegin) << "\n";
				return ss.str();
			}
		};
		preframe_material_ = material.get();
	}

	PERF_MARKER("VolumetricCloud");

	{
		PERF_MARKER("Checkerboard Depth");
		GLBindTextures({ depth_texture });
		GLBindSamplers({ Samplers::GetNearestClampToEdge() });
		GLBindImageTextures({ viewport_data_->checkerboard_depth_.id() });

		glUseProgram(checkerboard_gen_program_.id());
		checkerboard_gen_program_.Dispatch(viewport_ / 2);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
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

	glm::vec2 additional_delta{0, 0};
	material->Setup(viewport_, camera, offset_from_first_, additional_delta);
	delta_local += glm::vec3(additional_delta, 0);

	VolumetricCloudCommonBufferData common_buffer;
	common_buffer.uInvMVP = glm::inverse(mvp);
	common_buffer.uPreMVP = pre_mvp;
	common_buffer.uCameraPos = inv_model * glm::vec4(camera_pos, 1.0f);
	common_buffer.uFrameDelta = delta_local;
	common_buffer.uBaseShadingIndex = frame_id_ & 0x3;
	common_buffer.uLinearDepthParam = {1.0f / camera.zNear, (camera.zFar - camera.zNear) / (camera.zFar * camera.zNear) };

	glNamedBufferSubData(common_buffer_.id(), 0, sizeof(common_buffer), &common_buffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, common_buffer_.id());

	{
		PERF_MARKER("Index Generate");
		GLBindTextures({ viewport_data_->checkerboard_depth_.id() });
		GLBindSamplers({ Samplers::GetNearestClampToEdge() });
		GLBindImageTextures({ viewport_data_->index_linear_depth_.id() });

		glUseProgram(index_gen_program_.id());
		index_gen_program_.Dispatch(viewport_ / 4);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	auto aerial_perspective = atmosphere_render.aerial_perspective_lut();

	VolumetricCloudBufferData buffer;
	buffer.uEarthRadius = earth_radius;
	buffer.uSunDirection = glm::normalize(glm::mat3(inv_model) * atmosphere_render.sun_direction());
	buffer.uBottomAltitude = bottom_altitude_;
	buffer.uTopAltitude = bottom_altitude_ + thickness_;
	buffer.uMaxRaymarchDistance = max_raymarch_distance_;
	buffer.uMaxRaymarchSteps = max_raymarch_steps_;
	buffer.uMaxVisibleDistance = max_visible_distance_;
	buffer.uEnvColorScale = env_color_ * env_color_scale_;
	buffer.uSunIlluminanceScale = sun_illuminance_scale_;
	buffer.uShadowSteps = shadow_steps_;
	buffer.uShadowDistance = shadow_distance_;
	buffer.uAerialPerspectiveLutMaxDistance = aerial_perspective.max_distance;
	buffer.uFrameID = static_cast<float>(frame_id_);

	glNamedBufferSubData(buffer_.id(), 0, sizeof(buffer), &buffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, buffer_.id());

	offset_from_first_ += glm::dvec2(delta_local);
	frame_id_ = (frame_id_ + 1) & 0xff;
	camera_pos_ = camera_pos;
	mvp_ = mvp;

	{
		PERF_MARKER("Render");
		GLBindTextures({ viewport_data_->checkerboard_depth_.id(),
						viewport_data_->index_linear_depth_.id(),
						earth.atmosphere().transmittance_texture(),
						aerial_perspective.luminance_tex,
						aerial_perspective.transmittance_tex,
						Textures::Instance().blue_noise() });

		GLBindSamplers({ 0u,
						0u,
						Samplers::GetLinearNoMipmapClampToEdge(),
						Samplers::GetLinearNoMipmapClampToEdge(),
						Samplers::GetLinearNoMipmapClampToEdge(),
						0u, });

		GLBindImageTextures({ viewport_data_->render_texture_.id(),
							viewport_data_->cloud_distance_texture_.id() });

		glUseProgram(render_program_.id());
		render_program_.Dispatch(viewport_ / 4);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	{
		PERF_MARKER("Reconstruct");
		GLBindTextures<2>({// checkerboard_depth_.id(),
						// index_linear_depth_.id(),
						viewport_data_->render_texture_.id(),
						viewport_data_->cloud_distance_texture_.id(),
						viewport_data_->reconstruct_texture_[1].id(), });
		GLBindSamplers<2>({// 0u,
						// 0u,
						0u,
						0u,
						Samplers::GetLinearNoMipmapClampToEdge(), });
		GLBindImageTextures({ viewport_data_->reconstruct_texture_[0].id() });

		glUseProgram(reconstruct_program_.id());
		reconstruct_program_.Dispatch(viewport_ / 2);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	{
		PERF_MARKER("Upscale");
		GLBindTextures<1>({// checkerboard_depth_.id(),
						depth_texture,
						viewport_data_->reconstruct_texture_[0].id() });
		GLBindSamplers<1>({// 0u,
						Samplers::GetNearestClampToEdge(),
						0u });
		GLBindImageTextures({ hdr_texture });

		glUseProgram(upscale_program_.id());
		upscale_program_.Dispatch(viewport_);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	using std::swap;
	swap(viewport_data_->reconstruct_texture_[0], viewport_data_->reconstruct_texture_[1]);
}

void VolumetricCloud::DrawGUI() {
	ImGui::SliderFloat("Bottom Altitude", &bottom_altitude_, 0.0f, 10.0f);
	ImGui::SliderFloat("Thickness", &thickness_, 0.0f, 10.0f);
	ImGui::SliderFloat("Max Visible Distance", &max_visible_distance_, 0.0f, 1e5f, "%.0f", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("Max Raymarch Distance", &max_raymarch_distance_, 0.0f, 100.0f);
	ImGui::SliderFloat("Max Raymarch Steps", &max_raymarch_steps_, 0.0f, 256.0f);
	ImGui::ColorEdit3("Ambient Color", glm::value_ptr(env_color_));
	ImGui::SliderFloat("Ambient Color Scale", &env_color_scale_, 0.0f, 1.0f);
	ImGui::SliderFloat("Sun Illuminance Scale", &sun_illuminance_scale_, 0.0f, 1.0f);
	ImGui::SliderFloat("Shadow Steps", &shadow_steps_, 0.0f, 10.0f);
	ImGui::SliderFloat("Shadow Distance", &shadow_distance_, 0.0f, 10.0f);
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
