#pragma once

#include "vulkan_context.h"
#include <bitset>

enum class CursorMode : uint8_t {
	Normal,
	Hidden,
	Centered
};

class Mouse {
using Button = int;

private:
	GLFWwindow* m_sourceWindow;
	std::bitset<GLFW_MOUSE_BUTTON_LAST + 1> m_down, m_pressed, m_released;
	glm::vec2 m_position;
	glm::vec2 m_movement;

	void callback(GLFWwindow* window, Button btn, int action, int mods);

public:
	void listen(GLFWwindow* window);

	inline void resetButtonStates() { 
		m_pressed.reset(); 
		m_released.reset();
	}

	inline void updateCursorPosition() {
		
		auto prev = m_position;
		
		double x, y;
		glfwGetCursorPos(m_sourceWindow, &x, &y);
		m_position.x = static_cast<float>(x);
		m_position.y = static_cast<float>(y);

		m_movement = m_position - prev;
	}

	void setCursorMode(CursorMode mode);

	inline bool down(Button btn)     const noexcept { return m_down[btn];     }
	inline bool pressed(Button btn)  const noexcept { return m_pressed[btn];  }
	inline bool released(Button btn) const noexcept { return m_released[btn]; }
	
	GETTER(position, m_position);	
	GETTER(movement, m_movement);	
};
