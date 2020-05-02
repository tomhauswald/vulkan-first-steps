#include "../game_object.h"

#include <glm/gtx/rotate_vector.hpp>

class FirstPersonController : public GameObject {
private:
	static constexpr float pitchRadius = 89.9f;
	
    Keyboard const& m_kb;
	Mouse& m_mouse;
	Camera3d& m_cam;
	bool m_captureMouse;

	glm::vec3 m_eye;
	float m_pitch;
	float m_yaw;
	bool m_moving;
	bool m_sprinting;
	float m_moveSpeed;
	float m_sprintSpeed;
	float m_rotateSpeed;

public:
	FirstPersonController(Engine& e, float moveSpeed = 4.0f, float sprintSpeed = 8.0f, float rotateSpeed = 60.0f) : 
		GameObject(e),
		m_kb(e.renderer().keyboard()),
		m_mouse(e.renderer().mouse()),
		m_cam(e.renderer().camera3d()),
        m_captureMouse(true),
        m_eye(0.0f, 1.5f, 0.0f),
        m_pitch(0),
        m_yaw(0),
        m_moving(false),
        m_sprinting(false),
        m_moveSpeed(moveSpeed),
        m_sprintSpeed(sprintSpeed),
        m_rotateSpeed(rotateSpeed) {

		m_engine.renderer().mouse().setCursorMode(CursorMode::Centered);
	}
	
	void update(float dt) override {
		
		if (m_kb.pressed(GLFW_KEY_ESCAPE)) {
			m_captureMouse = !m_captureMouse;
			m_mouse.setCursorMode(
				m_captureMouse 
				? CursorMode::Centered 
				: CursorMode::Normal
			);
		}

		// Control camera with keyboard and mouse.
		if (m_captureMouse) {

			m_pitch = glm::clamp(
				m_pitch + m_mouse.movement().y * m_rotateSpeed * dt, 
				-pitchRadius, 
				pitchRadius
			);

			m_yaw += m_mouse.movement().x * m_rotateSpeed * dt;
			while (m_yaw >= 180.0f) m_yaw -= 360.0f;
			while (m_yaw < -180.0f) m_yaw += 360.0f;

			printf(
				"X: %0.1f Y: %0.1f Z: %0.1f Pitch: %0.1f Yaw: %0.1f\n", 
				m_eye.x, m_eye.y, m_eye.z, m_pitch, m_yaw
			);

			auto forward = glm::rotateY(glm::vec3{ 0, 0, 1 }, glm::radians(m_yaw));
			auto right = glm::cross(glm::vec3{ 0, 1, 0 }, forward);
			forward = glm::rotate(forward, glm::radians(m_pitch), right);

			auto dir = glm::vec3{};

			if (m_kb.down(GLFW_KEY_W)) dir += forward;
			if (m_kb.down(GLFW_KEY_A)) dir -= right;
			if (m_kb.down(GLFW_KEY_S)) dir -= forward;
			if (m_kb.down(GLFW_KEY_D)) dir += right;

			m_moving = (dir.x != 0.0f || dir.z != 0.0f);
			m_sprinting = m_moving && m_kb.down(GLFW_KEY_LEFT_SHIFT);

			if(m_moving) {
				m_eye += glm::normalize(dir) * (m_sprinting ? m_sprintSpeed : m_moveSpeed) * dt;
			}

			m_cam.setPosition(m_eye);
			m_cam.lookAt(m_eye + forward);
		}
		
		GameObject::update(dt);
	}
};

