#pragma once

#include "game_object.h"

class FramerateCounter : public GameObject {
private:
	size_t m_frames;
	float m_elapsed;
	float m_interval;

public:
	inline FramerateCounter(Engine& e, float interval = 5.0f) :
		GameObject(e),	
		m_frames{ 0 },
		m_elapsed{ 0.0f },
		m_interval{ interval } {
	}
	
	inline void update(float dt) override {
		m_elapsed += dt;
		m_frames++;
		if (m_elapsed >= m_interval) {
			std::cout << "[FramerateCounter] " << std::round(m_frames / m_elapsed) << " FPS." << lf;
			m_elapsed = 0.0f;
			m_frames = 0;
		}
		GameObject::update(dt);
	}
};
