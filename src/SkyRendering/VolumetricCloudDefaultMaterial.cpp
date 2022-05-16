#include "VolumetricCloudDefaultMaterial.h"

#include <imgui.h>

#include "ImageLoader.h"
#include "Samplers.h"
#include "ImGuiExt.h"

struct VolumetricCloudDefaultMaterial::BufferData {
	SampleInfo uCloudMapSampleInfo;
	SampleInfo uDetailSampleInfo;
	SampleInfo uDisplacementSampleInfo;
	glm::vec2 uDetailParam;
	float uLodBias;
	float uDensity;
	glm::vec3 padding0;
	float uDisplacementScale;
};

VolumetricCloudDefaultMaterial::VolumetricCloudDefaultMaterial() {
	{
		constexpr int w = 512;
		cloud_map_.texture.x = cloud_map_.texture.y = w;
		cloud_map_.texture.tex.Create(GL_TEXTURE_2D);
		glTextureStorage2D(cloud_map_.texture.id(), GetMipmapLevels(w, w), GL_RG8, w, w);
		cloud_map_.texture.repeat_size = 18.99f;

		cloud_map_.program = {
			"../shaders/SkyRendering/NoiseGen.comp",
			{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}, {8, 4}},
			[](const std::string& src) { return std::string("#version 460\n#define CLOUD_MAP_GEN\n") + src; },
			"CLOUD_MAP"
		};
	}
	{
		constexpr int w = 128;
		detail_.texture.x = detail_.texture.y = detail_.texture.z = w;
		detail_.texture.tex.Create(GL_TEXTURE_3D);
		glTextureStorage3D(detail_.texture.id(), GetMipmapLevels(w, w, w), GL_R8, w, w, w);
		detail_.texture.repeat_size = 5.33f;

		detail_.program = {
			"../shaders/SkyRendering/NoiseGen.comp",
			{{4, 4, 4}, {8, 4, 4}, {8, 8, 4}},
			[](const std::string& src) { return std::string("#version 460\n#define DETAIL_MAP_GEN\n") + src; },
			"DETAIL_MAP"
		};
	}
	{
		constexpr int w = 128;
		displacement_.texture.x = displacement_.texture.y = w;
		displacement_.texture.tex.Create(GL_TEXTURE_2D);
		glTextureStorage2D(displacement_.texture.id(), GetMipmapLevels(w, w), GL_RGBA8, w, w);
		displacement_.texture.repeat_size = 3.51f;

		displacement_.program = {
			"../shaders/SkyRendering/NoiseGen.comp",
			{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}, {8, 4}},
			[](const std::string& src) { return std::string("#version 460\n#define DISPLACEMENT_GEN\n") + src; },
			"DISPLACEMENT"
		};
	}

	buffer_.Create();
	glNamedBufferStorage(buffer_.id(), sizeof(BufferData), NULL, GL_DYNAMIC_STORAGE_BIT);
}

std::string VolumetricCloudDefaultMaterial::ShaderPath() {
	return "../shaders/SkyRendering/VolumetricCloudMaterial0.glsl";
}

void VolumetricCloudDefaultMaterial::Update(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) {
	cloud_map_.GenerateIfParameterChanged();
	detail_.GenerateIfParameterChanged();
	displacement_.GenerateIfParameterChanged();


	BufferData buffer;
	buffer.uLodBias = lod_bias_;
	buffer.uDensity = density_;
	buffer.uDetailParam = detail_param_;
	buffer.uDisplacementScale = displacement_scale_;

	auto gen_sample_info = [&viewport, &camera](const TextureWithInfo& tex, SampleInfo& info, glm::dvec2 offset) {
		info.frequency = 1.0f / tex.repeat_size;
		info.bias = glm::vec2(glm::fract(offset / static_cast<double>(tex.repeat_size)));
		info.k_lod = CalKLod(tex, viewport, camera);
	};
	const glm::vec2 kLocalWindDirection{ 1.0, 0.0 };
	additional_delta = kLocalWindDirection * wind_speed_ * ImGui::GetIO().DeltaTime;
	detail_offset_from_first_ += additional_delta * detail_wind_magnify_;
	gen_sample_info(cloud_map_.texture, buffer.uCloudMapSampleInfo, offset_from_first);
	gen_sample_info(detail_.texture, buffer.uDetailSampleInfo, offset_from_first + detail_offset_from_first_);
	gen_sample_info(displacement_.texture, buffer.uDisplacementSampleInfo, offset_from_first);

	glNamedBufferSubData(buffer_.id(), 0, sizeof(buffer), &buffer);
}

void VolumetricCloudDefaultMaterial::Bind() {
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, buffer_.id());

	GLBindTextures({ 
		cloud_map_.texture.id(),
		detail_.texture.id(),
		displacement_.texture.id(),
		}, kMaterialTextureUnitBegin);
	GLBindSamplers({ 
		Samplers::Get(Samplers::Wrap::REPEAT, Samplers::Mag::LINEAR, minfilter2d_),
		Samplers::Get(Samplers::Wrap::REPEAT, Samplers::Mag::LINEAR, minfilter3d_),
		Samplers::Get(Samplers::Wrap::REPEAT, Samplers::Mag::LINEAR, minfilter_displacement_),
		}, kMaterialTextureUnitBegin);
}

void VolumetricCloudDefaultMaterial::DrawGUI() {
	if (ImGui::TreeNode("Cloud Map")) {
		cloud_map_.buffer.DrawGUI();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Detail")) {
		detail_.buffer.DrawGUI();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Displacement")) {
		displacement_.buffer.DrawGUI();
		ImGui::TreePop();
	}
	EnumSelectable("Sampler 2D Filter", &minfilter2d_);
	EnumSelectable("Sampler 3D Filter", &minfilter3d_);
	EnumSelectable("Sampler Displacement Filter", &minfilter_displacement_);
	ImGui::SliderFloat("Lod Bias", &lod_bias_, -2.0f, 10.0f);
	ImGui::SliderFloat("Cloud Map Repeat Size", &cloud_map_.texture.repeat_size, 0.1f, 100.0f);
	ImGui::SliderFloat("Detail Repeat Size", &detail_.texture.repeat_size, 0.1f, 100.0f);
	ImGui::SliderFloat("Displacement Repeat Size", &displacement_.texture.repeat_size, 0.1f, 100.0f);
	ImGui::SliderFloat("Density", &density_, 0.0f, 50.0f);
	ImGui::SliderFloat("Wind Speed", &wind_speed_, 0.0f, 1.0f);
	ImGui::SliderFloat("Detail Wind Speed", &detail_wind_magnify_, 0.0f, 5.0f);
	ImGui::SliderFloat("Detail Param X", &detail_param_.x, -1.0f, 1.0f);
	ImGui::SliderFloat("Detail Param Y", &detail_param_.y, 0.0f, 1.0f);
	ImGui::SliderFloat("Displacement Scale", &displacement_scale_, 0.0f, 5.0f);
}

void VolumetricCloudDefaultMaterial::NoiseCreateInfo::DrawGUI() {
	ImGui::InputInt("Seed", &seed);
	ImGui::SliderInt("Base Frequency", &base_frequency, 1, 16);
	ImGui::SliderFloat("Remap Min", &remap_min, 0.0f, 1.0f);
	ImGui::SliderFloat("Remap Max", &remap_max, 0.0f, 1.0f);
}

void VolumetricCloudDefaultMaterial::CloudMapBuffer::DrawGUI() {
	if (ImGui::TreeNode("Density")) {
		uDensity.DrawGUI();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Height")) {
		uHeight.DrawGUI();
		ImGui::TreePop();
	}
}

void VolumetricCloudDefaultMaterial::DetailBuffer::DrawGUI() {
	if (ImGui::TreeNode("Perlin")) {
		uPerlin.DrawGUI();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Worley")) {
		uWorley.DrawGUI();
		ImGui::TreePop();
	}
}

void VolumetricCloudDefaultMaterial::DisplacementBuffer::DrawGUI() {
	uPerlin.DrawGUI();
}
