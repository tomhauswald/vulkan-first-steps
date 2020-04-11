#pragma once

#include "common.h"

class Renderer;

class GameObject {
private:
	bool m_alive;
	float m_lifetime;

public:
	inline GameObject() : 
		m_alive{ true },
		m_lifetime{ 0.0f } {
	}

	inline virtual ~GameObject() {
	}

	inline virtual void update(float dt) {
		m_lifetime += dt;
	}

	inline virtual void draw(Renderer& r) {
	}

	inline void kill() noexcept {
		m_alive = false;
	}

	GETTER(alive, m_alive)
	GETTER(lifetime, m_lifetime)
};
