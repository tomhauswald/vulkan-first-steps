#pragma once

#include "game_object.h"
#include "renderer.h"

class Sprite : public GameObject, public HwSpriteInfo {
public:
	inline Sprite(Texture const& texture) :
		GameObject{}, HwSpriteInfo(texture) {
	}
	
	inline virtual ~Sprite() {
	}
	
	inline virtual void update(float dt) override {
		GameObject::update(dt);
	}

	inline virtual void draw(Renderer& r) override {
		r.renderSprite(*this);
	}
};

class KinematicSprite : public Sprite {
private:
	glm::vec2 m_velocity;
	glm::vec2 m_acceleration;

public:
	inline KinematicSprite(Texture const& texture) :
		Sprite(texture), m_velocity{}, m_acceleration{} {
	}

	inline virtual ~KinematicSprite() {
	}

	inline virtual void update(float dt) override {
		setPosition(position() + m_velocity * dt);
		m_velocity += m_acceleration * dt;
		Sprite::update(dt);
	}

	GETTER(velocity, m_velocity)
	GETTER(acceleration, m_acceleration)

	SETTER(setVelocity, m_velocity)
	SETTER(setAcceleration, m_acceleration)
};
