#include "Mesh.h"

#include <assert.h>
#include <cmath>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

inline glm::vec3 GetAnyOrthogonalVector(const glm::vec3& N) {
    if (glm::abs(N.z) < 1e-6f)
        return { 0,0,1 };
    return glm::normalize(glm::vec3(1, 1, (-N.x - N.y) / N.z));
}

Mesh::Mesh(const MeshVertices& vertices) {
    assert(vertices.mode == GL_TRIANGLES
        || vertices.mode == GL_TRIANGLE_STRIP
        || vertices.mode == GL_TRIANGLE_FAN);
    assert(vertices.positions.size() == vertices.normals.size());

    auto vertices_num = vertices.positions.size();
    mode = vertices.mode;
    count = static_cast<GLsizei>(vertices.indices.size());
    constexpr int stride = 11;

    vao_.Create();
    vbo_.Create();
    ebo_.Create();
    std::vector<float> data;
    data.reserve(vertices_num * stride);
    for (int i = 0; i < vertices_num; ++i) {
        data.push_back(vertices.positions[i][0]);
        data.push_back(vertices.positions[i][1]);
        data.push_back(vertices.positions[i][2]);
        data.push_back(vertices.normals[i][0]);
        data.push_back(vertices.normals[i][1]);
        data.push_back(vertices.normals[i][2]);
        if (!vertices.uvs.empty()) {
            data.push_back(vertices.uvs[i][0]);
            data.push_back(vertices.uvs[i][1]);
            if (vertices.tangents.empty()) {
               // TODO: calculate tangents
                throw std::runtime_error("Tangents is empty");
            }
            data.push_back(vertices.tangents[i][0]);
            data.push_back(vertices.tangents[i][1]);
            data.push_back(vertices.tangents[i][2]);
        }
        else {
            data.push_back(0);
            data.push_back(0);
            auto tangent = GetAnyOrthogonalVector(glm::make_vec3(vertices.normals[i].data()));
            data.push_back(tangent[0]);
            data.push_back(tangent[1]);
            data.push_back(tangent[2]);
        }
    }
    glNamedBufferStorage(vbo_.id(), sizeof(data[0]) * data.size(), data.data(), 0);
    glNamedBufferStorage(ebo_.id(), sizeof(vertices.indices[0]) * vertices.indices.size(),
        vertices.indices.data(), 0);
    glBindVertexArray(vao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, vbo_.id());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_.id());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
}

void Mesh::Draw() const {
    glBindVertexArray(vao_.id());
    glDrawElements(mode, count, GL_UNSIGNED_INT, 0);
}

MeshVertices CreatePyramid() {
    MeshVertices vertices;
    vertices.mode = GL_TRIANGLES;
    vertices.positions = {
        {0.f, 1.f, 0.f}, {1.f, 0.f, 1.f}, {1.f, 0.f, -1.f},
        {0.f, 1.f, 0.f}, {1.f, 0.f, -1.f}, {-1.f, 0.f, -1.f},
        {0.f, 1.f, 0.f}, {-1.f, 0.f, -1.f}, {-1.f, 0.f, 1.f},
        {0.f, 1.f, 0.f}, {-1.f, 0.f, 1.f}, {1.f, 0.f, 1.f}};
    vertices.normals = {
        {1.f, 1.f, 0.f}, {1.f, 1.f, 0.f}, {1.f, 1.f, 0.f},
        {0.f, 1.f, -1.f}, {0.f, 1.f, -1.f}, {0.f, 1.f, -1.f},
        {-1.f, 1.f, 0.f}, {-1.f, 1.f, 0.f}, {-1.f, 1.f, 0.f},
        {0.f, 1.f, 1.f}, {0.f, 1.f, 1.f}, {0.f, 1.f, 1.f}};
    vertices.indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    return vertices;
}

MeshVertices CreateSphere() {
    // https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.2.lighting_textured/lighting_textured.cpp
    MeshVertices vertices;
    vertices.mode = GL_TRIANGLE_STRIP;

    auto pi = static_cast<float>(PI);
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = cos(xSegment * 2.0f * pi) * sin(ySegment * pi);
            float yPos = cos(ySegment * pi);
            float zPos = sin(xSegment * 2.0f * pi) * sin(ySegment * pi);

            float xTan = sin(xSegment * 2.0f * pi);
            float yTan = 0;
            float zTan = -cos(xSegment * 2.0f * pi);

            vertices.positions.push_back({ xPos, yPos, zPos });
            vertices.normals.push_back({ xPos, yPos, zPos });
            vertices.tangents.push_back({ xTan , yTan, zTan });
            vertices.uvs.push_back({ 1.0f - xSegment, 1.0f - ySegment });
        }
    }

    bool oddRow = false;
    for (int y = 0; y < Y_SEGMENTS; ++y) {
        if (!oddRow) {
            for (int x = 0; x <= X_SEGMENTS; ++x) {
                vertices.indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                vertices.indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        else {
            for (int x = X_SEGMENTS; x >= 0; --x) {
                vertices.indices.push_back(y * (X_SEGMENTS + 1) + x);
                vertices.indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    return vertices;
}

MeshVertices ReadMeshFile(const char* path) {
    MeshVertices vertices;
    vertices.mode = GL_TRIANGLES;

    std::ifstream in;
    in.open(path, std::ios::binary);
    if (!in)
        throw std::runtime_error(std::string("Mesh File Not Exist: ") + path);
    unsigned int vertices_count_;
    unsigned int indices_count_;
    in.read(reinterpret_cast<char*>(&vertices_count_), sizeof(vertices_count_));
    in.read(reinterpret_cast<char*>(&indices_count_), sizeof(indices_count_));
    vertices.positions.resize(vertices_count_);
    vertices.normals.resize(vertices_count_);
    vertices.indices.resize(indices_count_);
    in.read(reinterpret_cast<char*>(vertices.positions.data()), vertices_count_ * sizeof(vertices.positions[0]));
    in.read(reinterpret_cast<char*>(vertices.normals.data()), vertices_count_ * sizeof(vertices.normals[0]));
    in.read(reinterpret_cast<char*>(vertices.indices.data()), indices_count_ * sizeof(vertices.indices[0]));
    return vertices;
}
