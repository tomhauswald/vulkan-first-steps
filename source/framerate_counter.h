#pragma once

#include "game_object.h"

class FramerateCounter : public GameObject {
private:
	size_t m_frames;
	float m_seconds;
	float m_fps;

public:
	inline FramerateCounter() : 
		m_frames{ 0 },
		m_seconds{ 0.0f },
		m_fps{ 60.0f } {
	}
	
	inline virtual void update(float dt) override {
		m_seconds += dt;
		m_frames++;
		if (m_seconds >= 1.0f) {
			m_fps = m_frames / m_seconds;
			m_seconds = 0.0f;
			m_frames = 0;
		}
		GameObject::update(dt);
	}

	GETTER(fps, m_fps);

	virtual ~FramerateCounter() { }
};