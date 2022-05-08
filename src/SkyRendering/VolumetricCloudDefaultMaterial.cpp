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
		cloud_map_.x = cloud_map_.y = 512;
		cloud_map_.tex.Create(GL_TEXTURE_2D);
		glTextureStorage2D(cloud_map_.id(), GetMipmapLevels(cloud_map_.x, cloud_map_.y), GL_RG8, cloud_map_.x, cloud_map_.y);
		cloud_map_.repeat_size = 18.99f;

		cloud_map_gen_program_ = {
			"../shaders/SkyRendering/NoiseGen.comp",
			{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}, {8, 4}},
			[] { return std::string("#version 460\n#define CLOUD_MAP_GEN\n"); }
		};
		GLBindImageTextures({ cloud_map_.id() });
		glUseProgram(cloud_map_gen_program_.id());
		cloud_map_gen_program_.Dispatch(glm::ivec2(cloud_map_.x, cloud_map_.y));
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateTextureMipmap(cloud_map_.id());
	}
	{
		constexpr int w = 128;
		detail_texture_.x = detail_texture_.y = detail_texture_.z = w;
		detail_texture_.tex.Create(GL_TEXTURE_3D);
		glTextureStorage3D(detail_texture_.id(), GetMipmapLevels(w, w, w), GL_R8, w, w, w);
		detail_texture_.repeat_size = 5.33f;

		detail_gen_program_ = {
			"../shaders/SkyRendering/NoiseGen.comp",
			{{4, 4, 4}, {8, 4, 4}, {8, 8, 4}},
			[] { return std::string("#version 460\n#define DETAIL_MAP_GEN\n"); }
		};
		GLBindImageTextures({ detail_texture_.id() });
		glUseProgram(detail_gen_program_.id());
		detail_gen_program_.Dispatch(glm::ivec3(w, w, w));
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateTextureMipmap(detail_texture_.id());
	}
	{
		displacement_texture_.x = displacement_texture_.y = 128;
		displacement_texture_.tex.Create(GL_TEXTURE_2D);
		glTextureStorage2D(displacement_texture_.id(), GetMipmapLevels(displacement_texture_.x, displacement_texture_.y), GL_RGBA8, displacement_texture_.x, displacement_texture_.y);
		displacement_texture_.repeat_size = 3.51f;

		displacement_gen_program_ = {
			"../shaders/SkyRendering/NoiseGen.comp",
			{{16, 8}, {32, 16}, {32, 32}, {8, 8}, {16, 16}, {8, 4}},
			[] { return std::string("#version 460\n#define DISPLACEMENT_GEN\n"); }
		};
		GLBindImageTextures({ displacement_texture_.id() });
		glUseProgram(displacement_gen_program_.id());
		displacement_gen_program_.Dispatch(glm::ivec2(displacement_texture_.x, displacement_texture_.y));
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateTextureMipmap(displacement_texture_.id());
	}

	buffer_.Create();
	glNamedBufferStorage(buffer_.id(), sizeof(BufferData), NULL, GL_DYNAMIC_STORAGE_BIT);
}

std::string VolumetricCloudDefaultMaterial::ShaderPath() {
	return "../shaders/SkyRendering/VolumetricCloudMaterial0.comp";
}

void VolumetricCloudDefaultMaterial::Setup(glm::vec2 viewport, const Camera& camera, const glm::dvec2& offset_from_first, glm::vec2& additional_delta) {
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
	gen_sample_info(cloud_map_, buffer.uCloudMapSampleInfo, offset_from_first);
	gen_sample_info(detail_texture_, buffer.uDetailSampleInfo, offset_from_first + detail_offset_from_first_);
	gen_sample_info(displacement_texture_, buffer.uDisplacementSampleInfo, offset_from_first);

	glNamedBufferSubData(buffer_.id(), 0, sizeof(buffer), &buffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, buffer_.id());

	GLBindTextures({ 
		cloud_map_.id(),
		detail_texture_.id(),
		displacement_texture_.id(),
		}, kMaterialTextureUnitBegin);
	GLBindSamplers({ 
		Samplers::Get(Samplers::Wrap::REPEAT, Samplers::Mag::LINEAR, minfilter2d_),
		Samplers::Get(Samplers::Wrap::REPEAT, Samplers::Mag::LINEAR, minfilter3d_),
		Samplers::Get(Samplers::Wrap::REPEAT, Samplers::Mag::LINEAR, minfilter_displacement_),
		}, kMaterialTextureUnitBegin);
}

void VolumetricCloudDefaultMaterial::DrawGUI() {
	EnumSelectable("Sampler 2D Filter", &minfilter2d_);
	EnumSelectable("Sampler 3D Filter", &minfilter3d_);
	EnumSelectable("Sampler Displacement Filter", &minfilter_displacement_);
	ImGui::SliderFloat("Lod Bias", &lod_bias_, -2.0f, 10.0f);
	ImGui::SliderFloat("Cloud Map Repeat Size", &cloud_map_.repeat_size, 0.1f, 100.0f);
	ImGui::SliderFloat("Detail Repeat Size", &detail_texture_.repeat_size, 0.1f, 100.0f);
	ImGui::SliderFloat("Displacement Repeat Size", &displacement_texture_.repeat_size, 0.1f, 100.0f);
	ImGui::SliderFloat("Density", &density_, 0.0f, 50.0f);
	ImGui::SliderFloat("Wind Speed", &wind_speed_, 0.0f, 1.0f);
	ImGui::SliderFloat("Detail Wind Speed", &detail_wind_magnify_, 0.0f, 5.0f);
	ImGui::SliderFloat("Detail Param X", &detail_param_.x, -1.0f, 1.0f);
	ImGui::SliderFloat("Detail Param Y", &detail_param_.y, 0.0f, 1.0f);
	ImGui::SliderFloat("Displacement Scale", &displacement_scale_, 0.0f, 5.0f);
}
