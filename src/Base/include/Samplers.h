#pragma once

#include <glad/glad.h>

#include "Singleton.h"

class Samplers :public Singleton<Samplers> {
public:
	friend Singleton<Samplers>;

	void SetAnisotropyEnable(bool enable);

	GLuint linear_clamp_to_edge() const {
		return linear_clamp_to_edge_;
	}

	GLuint linear_no_mipmap_clamp_to_edge() const {
		return linear_no_mipmap_clamp_to_edge_;
	}

	GLuint shadow_map_sampler() const {
		return shadow_map_sampler_;
	}

private:
	Samplers();
	~Samplers();

	GLuint linear_clamp_to_edge_;
	GLuint linear_no_mipmap_clamp_to_edge_;
	GLuint shadow_map_sampler_;
};

class Textures :public Singleton<Textures> {
public:
	friend Singleton<Textures>;

	GLuint blue_noise_texture() const {
		return blue_noise_texture_;
	}

	GLuint moon_albedo_texture() const {
		return moon_albedo_texture_;
	}

	GLuint star_texture() const {
		return star_texture_;
	}

	GLuint earth_texture() const {
		return earth_texture_;
	}

private:
	Textures();
	~Textures();

	GLuint blue_noise_texture_;
	GLuint moon_albedo_texture_;
	GLuint star_texture_;
	GLuint earth_texture_;
};



