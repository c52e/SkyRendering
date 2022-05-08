#include "GLProgram.h"

#include <iostream>
#include <array>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <regex>

#include "Utils.h"

namespace {

class Shader {
public:
	Shader(GLenum type, const char* src, const char* identifier) {
		id_ = glCreateShader(type);
		glShaderSource(id_, 1, &src, nullptr);
		glCompileShader(id_);

		GLint success;
		std::array<char, 1024> info{};
		glGetShaderiv(id_, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(id_, static_cast<GLsizei>(info.size()), nullptr, info.data());
			glDeleteShader(id_);
			throw std::runtime_error(std::string("Error while compiling ") 
				+ identifier + ":\n" + info.data());
		}
	}

	~Shader() {
		if (id_ != 0)
			glDeleteShader(id_);
	}

	GLuint Id()const { return id_; }

private:
	GLuint id_ = 0;
};

GLuint CreateProgram(const std::vector<GLuint>& shaders) {
	auto program = glCreateProgram();
	for (auto shader : shaders)
		glAttachShader(program, shader);
	glLinkProgram(program);

	GLint success;
	std::array<char, 1024> info{};
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, static_cast<GLsizei>(info.size()), NULL, info.data());
		glDeleteProgram(program);
		throw std::runtime_error(std::string("Error while linking program:\n") + info.data());
	}
	return program;
}

}

GLProgram::GLProgram(const char* vertex_src, const char* fragment_src, GLuint external_fragment_shader) {
#define STR(NAME) #NAME
	Shader vertex(GL_VERTEX_SHADER, vertex_src, STR(GL_VERTEX_SHADER));
	Shader fragment(GL_FRAGMENT_SHADER, fragment_src, STR(GL_FRAGMENT_SHADER));
	
	std::vector shaders{ vertex.Id(), fragment.Id() };
	if (external_fragment_shader != 0)
		shaders.push_back(external_fragment_shader);
	id_ = CreateProgram(shaders);
}

GLProgram::GLProgram(const char* compute_src, GLuint external_compute_shader) {
	Shader compute(GL_COMPUTE_SHADER, compute_src, STR(GL_COMPUTE_SHADER));

	std::vector shaders{ compute.Id() };
	if (external_compute_shader != 0)
		shaders.push_back(external_compute_shader);
	id_ = CreateProgram(shaders);
}

GLProgram::~GLProgram() {
	if (id_ != 0)
		glDeleteProgram(id_);
}

std::string Replace(std::string src, const std::string& from, const std::string& to) {
	for (;;) {
		auto index = src.find(from);
		if (index == std::string::npos)
			break;
		src.replace(index, from.size(), to);
	}
	return src;
}

std::string ReadWithPreprocessor(const char* filepath) {
	namespace fs = std::filesystem;
	fs::path fpath = filepath;
	auto dir = fpath.parent_path();
	auto src = ReadFile(filepath);
	std::regex pattern("#include[ \t]*\"(.+)\"");
	std::smatch match;
	while (std::regex_search(src, match, pattern)) {
		auto header_path = dir / match.str(1);
		auto header = ReadWithPreprocessor(header_path.string().c_str());
		src = src.substr(0, match.position()) + header 
			+ src.substr(match.position() + match.length(), src.size() - match.position() - match.length());
	}
	return src;
}
