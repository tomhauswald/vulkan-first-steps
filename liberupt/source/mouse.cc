#include "mouse.h"

void Mouse::callback(GLFWwindow* window, MouseButton btn, int action, int mods) {
	switch(action) {
	case GLFW_PRESS: 
		m_down[btn] = true; 
		m_pressed[btn] = true;
	break;
	
	case GLFW_RELEASE:
		m_down[btn] = false; 
		m_released[btn] = true;
	break;

	default: break;
	}
}

void Mouse::listen(GLFWwindow* window) {
	
	static Mouse* instance = nullptr;
	crashIf(instance);
	instance = this;

	glfwSetMouseButtonCallback(
		window, 
		[](GLFWwindow* window, int btn, int action, int mods) {
			instance->callback(window, btn, action, mods);
		}
	);

	m_pWindow = window;
}

void Mouse::setCursorMode(CursorMode mode) {

	auto glfwCursorMode = GLFW_CURSOR_NORMAL;
	auto glfwEnableRawMotion = GLFW_FALSE;

	if(mode == CursorMode::Hidden) {
		glfwCursorMode = GLFW_CURSOR_HIDDEN;
	} else if(mode == CursorMode::Centered) {
		glfwCursorMode = GLFW_CURSOR_DISABLED;
		glfwEnableRawMotion = glfwRawMouseMotionSupported();
	}
	
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, glfwCursorMode);
	glfwSetInputMode(m_pWindow, GLFW_RAW_MOUSE_MOTION, glfwEnableRawMotion);
}
