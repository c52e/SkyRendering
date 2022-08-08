#pragma once

#include "gl.hpp"
#include "Singleton.h"

class Textures :public Singleton<Textures> {
public:
	friend Singleton<Textures>;

	GLuint white(GLuint id = 0) const {
		return id ? id : white_.id();
	}

	GLuint normal(GLuint id = 0) const {
		return id ? id : normal_.id();
	}

	GLuint blue_noise() const {
		return blue_noise_.id();
	}

	GLuint moon_albedo() const {
		return moon_albedo_.id();
	}

	GLuint moon_normal() const {
		return moon_normal_.id();
	}

	GLuint star_luminance() const {
		return star_luminance_.id();
	}

	GLuint earth_albedo() const {
		return earth_albedo_.id();
	}

private:
	Textures();

	GLTexture white_;
	GLTexture normal_;
	GLTexture blue_noise_;
	GLTexture moon_albedo_;
	GLTexture moon_normal_;
	GLTexture star_luminance_;
	GLTexture earth_albedo_;
};



