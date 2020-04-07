#pragma once

#include <glm/gtx/transform.hpp>

class Camera2d {
private:
	glm::vec2 m_position;
	glm::vec2 m_viewportSize;
	float m_zoom;

public:
	inline Camera2d(glm::vec2 viewportSize) :
		m_position{ 0,0 },
		m_viewportSize{ std::move(viewportSize) },
		m_zoom{ 1 }{
	}

	inline glm::mat4 transform() const {
		return glm::orthoLH(0.0f, m_viewportSize.x, 0.0f, m_viewportSize.y, 0.0f, 1.0f);
	}
};

class Camera3d {
private:
	glm::vec3 m_position;
	glm::vec3 m_direction;
	float m_aspectRatio;
	float m_fovDegrees;
	glm::mat4 m_projection;

public:
	inline Camera3d(float aspectRatio, float fovDegrees) :
		m_position{ 0, 0, 0 },
		m_direction{ 0, 0, 1 },
		m_aspectRatio{ aspectRatio },
		m_fovDegrees{ fovDegrees }{

		m_projection = glm::perspectiveLH(
			glm::radians(m_fovDegrees),
			m_aspectRatio,
			1e-3f,
			1e3f
		);
	}

	GETTER(fovDegrees, m_fovDegrees)
	GETTER(position, m_position)
	GETTER(direction, m_direction)
	GETTER(aspectRatio, m_aspectRatio)

	SETTER(setPosition, m_position)
	SETTER(setDirection, m_direction)

	inline void lookAt(glm::vec3 const& target) noexcept {
		m_direction = glm::normalize(target - m_position);
	}

	inline glm::mat4 transform() const noexcept {
		return m_projection * glm::lookAtLH(
			m_position,
			m_position + m_direction,
			{ 0,1,0 }
		);
	}
};