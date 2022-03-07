#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Utils.h"

Camera::Camera(glm::vec3 position, float yaw, float pitch, float aspect)
	: position_(position), up_(kWorldUp), yaw_(yaw), pitch_(pitch), aspect_(aspect) {
	UpdateVectors();
}

Camera::~Camera() {
}

glm::mat4 Camera::ViewMatrix() const {
	return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::ProjectionMatrix() const {
	return glm::perspective(glm::radians(fovy_), aspect_, zNear_, zFar_);
}

glm::mat4 Camera::ViewMatrixVRLeft() const {
	auto pleft = PositionVRLeft();
	return glm::lookAt(pleft, pleft + front_, up_);
}

glm::mat4 Camera::ViewMatrixVRRight() const {
	auto pright = PositionVRRight();
	return glm::lookAt(pright, pright + front_, up_);
}

glm::mat4 Camera::ProjectionMatrixVR() const {
	return glm::perspective(glm::radians(fovy_), aspect_ * 0.5f, zNear_, zFar_);
}

void Camera::Rotate(float dPitch, float dYaw) {
	pitch_ += dPitch;
	yaw_ += dYaw;

	if (pitch_ > 89.0f)
		pitch_ = 89.0f;
	else if (pitch_ < -89.0f)
		pitch_ = -89.0f;

	UpdateVectors();
}

void Camera::UpdateVectors() {
	FromThetaPhiToDirection(glm::radians(90.f - pitch_), glm::radians(yaw_), glm::value_ptr(front_));
	right_ = glm::normalize(glm::cross(front_, kWorldUp));
	up_ = glm::normalize(glm::cross(right_, front_));
}
