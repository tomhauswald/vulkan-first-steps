#include "camera.h"

Camera::Camera(float aspectRatio, float fovDegrees) :
	m_position{ 0.0f },
	m_direction{ 0.0f, 0.0f, 1.0f },
	m_aspectRatio{ aspectRatio },
	m_fovDegrees{ fovDegrees },
	m_viewMatrix{ 1.0f } {

	m_projMatrix = glm::perspectiveLH(
		glm::radians(45.0f),
		m_aspectRatio,
		1e-3f,
		1e3f
	);
}

glm::vec3 const& Camera::position() const { return m_position; }
void Camera::setPosition(glm::vec3 const& pos) { m_position = pos; }

glm::vec3 const& Camera::direction() const { return m_direction; }
void Camera::setDirection(glm::vec3 const& dir) { m_direction = dir; }

void Camera::lookAt(glm::vec3 const& target) {
	m_direction = glm::normalize(target - m_position);
}

float Camera::aspectRatio() const { return m_aspectRatio; }
float Camera::fovDegrees() const { return m_fovDegrees; }

glm::mat4 const& Camera::viewMatrix() const {
	return glm::lookAtLH(
		m_position,
		m_position + m_direction,
		{ 0,1,0 }
	);
}

glm::mat4 const& Camera::projMatrix() const {
	return m_projMatrix;
}
