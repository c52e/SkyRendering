#pragma once

#include <magic_enum.hpp>

#include "gl.hpp"
#include "Singleton.h"

class Samplers :private Singleton<Samplers> {
public:
	friend Singleton<Samplers>;

	enum class Wrap {
		CLAMP_TO_EDGE = 0,
		REPEAT,
	};
	enum class Mag {
		NEAREST = 0,
		LINEAR,
	};
	enum class MipmapMin {
		NEAREST_MIPMAP_NEAREST = 0,
		LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR,
	};
	enum class NomipmapMin {
		NEAREST = 0,
		LINEAR,
	};
	static constexpr int kWrapCount = static_cast<int>(magic_enum::enum_count<Wrap>());
	static constexpr int kMagCount = static_cast<int>(magic_enum::enum_count<Mag>());
	static constexpr int kMipmapMinCount = static_cast<int>(magic_enum::enum_count<MipmapMin>());
	static constexpr int kNomipmapMinCount = static_cast<int>(magic_enum::enum_count<NomipmapMin>());

	static GLuint GetLinearNoMipmapClampToEdge() {
		return Get(Wrap::CLAMP_TO_EDGE, Mag::LINEAR, NomipmapMin::LINEAR);
	}

	static GLuint GetNearestClampToEdge() {
		return Get(Wrap::CLAMP_TO_EDGE, Mag::NEAREST, NomipmapMin::NEAREST);
	}

	static GLuint Get(Wrap wrap, Mag magfilter, MipmapMin minfilter) {
		return Instance().mipmap_[toint(wrap)][toint(magfilter)][toint(minfilter)].id();
	}
	
	static GLuint Get(Wrap wrap, Mag magfilter, NomipmapMin minfilter) {
		return Instance().nomipmap_[toint(wrap)][toint(magfilter)][toint(minfilter)].id();
	}
	
	static GLuint GetAnisotropySampler(Wrap wrap) {
		if (Instance().anisotropy_enable_)
			return Instance().anisotropy_[toint(wrap)].id();
		else
			return Get(wrap, Mag::LINEAR, MipmapMin::LINEAR_MIPMAP_LINEAR);
	}

	static GLuint GetShadowMapSampler() {
		return Instance().shadow_map_.id();
	}

	static void SetAnisotropyEnable(bool enable) {
		Instance().anisotropy_enable_ = enable;
	}

private:
	Samplers();

	template<class T>
	static int toint(T x) { return static_cast<int>(x); }

	bool anisotropy_enable_ = false;

	GLSampler mipmap_[kWrapCount][kMagCount][kMipmapMinCount];
	GLSampler nomipmap_[kWrapCount][kMagCount][kNomipmapMinCount];
	GLSampler anisotropy_[kWrapCount];
	GLSampler shadow_map_;
};
