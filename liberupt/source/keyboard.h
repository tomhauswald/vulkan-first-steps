#pragma once

#include <bitset>

#include "vulkan_context.h"

using Key = int;  // GLFW_KEY_

class Keyboard {
 private:
  std::bitset<GLFW_KEY_LAST + 1> m_down, m_pressed, m_released;

  void callback(GLFWwindow* window, Key key, int scancode, int action,
                int mods);

 public:
  void listen(GLFWwindow* window);

  inline void resetKeyStates() {
    m_pressed.reset();
    m_released.reset();
  }

  inline bool down(Key key) const noexcept { return m_down[key]; }
  inline bool pressed(Key key) const noexcept { return m_pressed[key]; }
  inline bool released(Key key) const noexcept { return m_released[key]; }
};
