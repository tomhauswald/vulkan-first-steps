#pragma once

#include "vulkan_context.h"
#include <bitset>

enum class CursorMode : uint8_t {
	Normal,
	Hidden,
	Centered
};

using MouseButton = int; // GLFW_MOUSE_BUTTON_

class Mouse {

private:
	GLFWwindow* m_pWindow;
	std::bitset<GLFW_MOUSE_BUTTON_LAST + 1> m_down;
	std::bitset<GLFW_MOUSE_BUTTON_LAST + 1> m_pressed;
	std::bitset<GLFW_MOUSE_BUTTON_LAST + 1> m_released;
	glm::vec2 m_position;
	glm::vec2 m_movement;
	bool m_isFirstFrame;

	void callback(GLFWwindow* window, MouseButton btn, int action, int mods);

public:
	inline Mouse() : 
		m_pWindow(nullptr),
		m_down(),
		m_pressed(),
		m_released(),
		m_position(0, 0),
		m_movement(0, 0),
		m_isFirstFrame(true) {}
	
	void listen(GLFWwindow* window);

	inline void resetButtonStates() { 
		m_pressed.reset(); 
		m_released.reset();
	}

	inline void updateCursorPosition() {
		
		auto prev = m_position;

		double x, y;
		glfwGetCursorPos(m_pWindow, &x, &y);
		m_position.x = static_cast<float>(x);
		m_position.y = static_cast<float>(y);

		if (!m_isFirstFrame) [[likely]] m_movement = m_position - prev;
		m_isFirstFrame = false;
	}

	void setCursorMode(CursorMode mode);

	inline bool down(MouseButton btn) const noexcept { return m_down[btn]; }
	inline bool pressed(MouseButton btn) const noexcept { return m_pressed[btn]; }
	inline bool released(MouseButton btn) const noexcept { return m_released[btn]; }
	
	GETTER(position, m_position);	
	GETTER(movement, m_movement);	
};
