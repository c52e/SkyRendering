#pragma once

#include <vector>
#include <array>

#include "gl.hpp"

struct MeshVertices {
	std::vector<std::array<float, 3>> positions;
	std::vector<std::array<float, 3>> normals;
	std::vector<std::array<float, 2>> uvs;
	std::vector<unsigned int> indices;
	GLenum mode;
};

class Mesh {
public:
	Mesh(const MeshVertices& vertices);

	void Draw() const;

private:
	GLsizei count;
	GLenum mode;

	GLVertexArray vao_;
	GLBuffer vbo_;
	GLBuffer ebo_;

};

MeshVertices CreatePyramid();

MeshVertices CreateSphere();

MeshVertices ReadMeshFile(const char* path);
