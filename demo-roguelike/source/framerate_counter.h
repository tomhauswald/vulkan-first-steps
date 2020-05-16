#pragma once

#include <engine.h>

class FramerateCounter : public GameObject3d {
 private:
  size_t m_frames = 0;
  float m_elapsed = 0;
  float m_interval;

 public:
  inline FramerateCounter(Engine3d& e, float interval = 5.0f)
      : GameObject3d(e), m_interval(interval) {}

  inline void update(float dt) override {
    m_elapsed += dt;
    m_frames++;
    if (m_elapsed >= m_interval) {
      std::cout << "[FramerateCounter] " << std::round(m_frames / m_elapsed)
                << " FPS." << lf;
      m_elapsed = 0.0f;
      m_frames = 0;
    }
    GameObject3d::update(dt);
  }
};
