#pragma once

#include <glm/gtx/transform.hpp>

class Camera {
private:
	glm::vec3 m_position;
	glm::vec3 m_direction;
	float m_aspectRatio;
	float m_fovDegrees;

	glm::mat4 m_viewMatrix;
	glm::mat4 m_projMatrix;

public:
	Camera(float aspectRatio = 16.0f / 9.0f, float fovDegrees = 45.0f);

	glm::vec3 const& position() const;
	void setPosition(glm::vec3 const& pos);

	glm::vec3 const& direction() const;
	void setDirection(glm::vec3 const& dir);

	void lookAt(glm::vec3 const& target);

	float aspectRatio() const;
	float fovDegrees() const;
	
	glm::mat4 const& viewMatrix() const;
	glm::mat4 const& projMatrix() const;
};