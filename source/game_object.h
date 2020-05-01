#pragma once

#include "common.h"

class Renderer;
class Engine;

class GameObject {
private:
	bool m_alive;
	float m_lifetime;

protected:
	Engine& m_engine;

public:
	inline GameObject(Engine& e) : 
		m_alive(true),
		m_lifetime(0.0f),
		m_engine(e) {}

	inline virtual ~GameObject() {}

	inline virtual void update(float dt) {
		m_lifetime += dt;
	}

	inline virtual void draw(Renderer& r) const {}

	inline void kill() noexcept {
		m_alive = false;
	}

	GETTER(alive, m_alive)
	GETTER(lifetime, m_lifetime)
};
