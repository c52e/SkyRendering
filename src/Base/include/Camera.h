#pragma once

#include <glm/glm.hpp>

#include "Serialization.h"

// 修改kWorldUp需要同时修改UpdateVectors()
inline const glm::vec3 kWorldUp{ 0, 1, 0 };

class Camera : public ISerializable {
public:
	Camera() = default;

	Camera(glm::vec3 position, float yaw, float pitch, float aspect);

	glm::mat4 ViewMatrix() const;

	glm::mat4 ProjectionMatrix() const;

	glm::mat4 ViewProjection() const { return ProjectionMatrix()* ViewMatrix(); }

	glm::vec3 position() const { return position_; }

	void set_position(glm::vec3 position) { position_ = position; }

	glm::vec3 front() const { return front_; }

	glm::vec3 right() const { return right_; }

	glm::vec3 up() const { return up_; }

	float aspect() const { return aspect_; }

	void set_aspect(float aspect) { aspect_ = aspect; }

	void Rotate(float dPitch, float dYaw);

	float fovy = 45.f;
	float zNear = 1e-1f;
	float zFar = 1e3f;

	FIELD_DECLARATION_BEGIN(ISerializable)
		FIELD_DECLARE(fovy)
		FIELD_DECLARE(zNear)
		FIELD_DECLARE(zFar)
		FIELD_DECLARE(position_)
		FIELD_DECLARE(front_)
		FIELD_DECLARE(right_)
		FIELD_DECLARE(up_)
		FIELD_DECLARE(yaw_)
		FIELD_DECLARE(pitch_)
		FIELD_DECLARE(aspect_)
	FIELD_DECLARATION_END()

private:
	void UpdateVectors();

	glm::vec3 position_;
	glm::vec3 front_;
	glm::vec3 right_;
	glm::vec3 up_;

	float yaw_{};
	float pitch_{};
	float aspect_{ 1.0f };
};

