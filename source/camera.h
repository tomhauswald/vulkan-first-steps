#pragma once

#include <glm/gtx/transform.hpp>

class Camera2d {
private:
	glm::vec2 m_position;
	glm::vec2 m_viewportSize;
	glm::vec2 m_viewportHalfSize;
	float m_zoom;

public:
	inline Camera2d(glm::vec2 viewportSize) :
		m_position{ 0,0 },
		m_viewportSize{ std::move(viewportSize) },
		m_viewportHalfSize{ m_viewportSize / 2.0f },
		m_zoom{ 1 }{
	}

	inline bool isScreenPointVisible(glm::vec2 const& point) const noexcept {
		return (
			(point.x > m_position.x) &&
			(point.y > m_position.y) &&
			(point.x < m_position.x + m_viewportSize.x) &&
			(point.y < m_position.y + m_viewportSize.y)
		);
	}

	inline glm::vec2 screenToViewportPoint(glm::vec2 const& point) const noexcept {
		return {
			point.x - m_position.x,
			point.y - m_position.y
		};
	}

	inline glm::vec2 screenToNdcPoint(glm::vec2 const& point) const noexcept {
		auto vpp = screenToViewportPoint(point) / m_viewportHalfSize;
		return { vpp.x - 1.0f, 1.0f - vpp.y };
	}

	inline glm::vec4 screenToNdcRect(glm::vec2 const& pos, glm::vec2 const& size) const noexcept {
		return {
			pos.x / m_viewportHalfSize.x - 1.0f,
			1.0f - pos.y / m_viewportHalfSize.y,
			size.x / m_viewportHalfSize.x,
			-size.y / m_viewportHalfSize.y
		};
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