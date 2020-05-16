#include "keyboard.h"

void Keyboard::callback(
	GLFWwindow* window, Key key, int scancode, int action, int mods
) {
	switch(action) {
		case GLFW_PRESS: 
			m_down[key] = true; 
			m_pressed[key] = true;
		break;
	
		case GLFW_RELEASE:
			m_down[key] = false; 
			m_released[key] = true;
		break;

		default: break;
	}
}

void Keyboard::listen(GLFWwindow* window) {
	
	static Keyboard* instance = nullptr;
	crashIf(instance);
	instance = this;

	glfwSetKeyCallback(
		window, 
		[](GLFWwindow* window, int key, int scancode, int action, int mods) {
			instance->callback(window, key, scancode, action, mods);
		}
	);
}

