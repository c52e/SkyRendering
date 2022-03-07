#pragma once

#include <glm/glm.hpp>

// 修改kWorldUp需要同时修改UpdateVectors()
inline const glm::vec3 kWorldUp{ 0, 1, 0 };

class Camera {
public:
	Camera(glm::vec3 position, float yaw, float pitch, float aspect);

	~Camera();

	glm::mat4 ViewMatrix() const;

	glm::mat4 ProjectionMatrix() const;

	glm::mat4 ViewProjection() const { return ProjectionMatrix()* ViewMatrix(); }

	glm::mat4 ViewMatrixVRLeft() const;

	glm::mat4 ViewMatrixVRRight() const;

	glm::mat4 ProjectionMatrixVR() const;

	glm::mat4 ViewProjectionVRLeft() const { return ProjectionMatrixVR() * ViewMatrixVRLeft(); }

	glm::mat4 ViewProjectionVRRight() const { return ProjectionMatrixVR() * ViewMatrixVRRight(); }

	glm::vec3 position() const { return position_; }

	glm::vec3 PositionVRLeft() const { return position_ - right_ * 0.5f * eye_distance_; }

	glm::vec3 PositionVRRight() const { return position_ + right_ * 0.5f * eye_distance_; }

	void set_position(glm::vec3 position) { position_ = position; }

	glm::vec3 front() const { return front_; }

	glm::vec3 right() const { return right_; }

	void set_z_near(float zNear) { zNear_ = zNear; }

	void set_z_far(float zFar) { zFar_ = zFar; }

	void set_aspect(float aspect) { aspect_ = aspect; }

	void set_fovy(float fovy) { fovy_ = fovy; }

	void set_eye_distance(float eye_distance) { eye_distance_ = eye_distance; }

	void Rotate(float dPitch, float dYaw);

private:
	void UpdateVectors();

	glm::vec3 position_;
	glm::vec3 front_;
	glm::vec3 right_;
	glm::vec3 up_;

	float yaw_;
	float pitch_;

	float fovy_ = 45.f;
	float aspect_;
	float zNear_ = 1e-1f;
	float zFar_ = 1e3f;
	float eye_distance_ = 1e-1f;
};

