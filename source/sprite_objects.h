#pragma once

#include "game_object.h"
#include "renderer.h"

class StationarySpriteObj : public GameObject, public Sprite {
public:
	inline StationarySpriteObj(Texture const& texture) :
		GameObject{}, Sprite(texture) {
	}
	
	inline virtual void update(float dt) override {
		GameObject::update(dt);
	}

	inline virtual void draw(Renderer& r) override {
		r.renderSprite(*this);
	}
};

class KinematicSpriteObj : public StationarySpriteObj {
private:
	glm::vec2 m_velocity;
	glm::vec2 m_acceleration;

public:
	inline KinematicSpriteObj(Texture const& texture) :
		StationarySpriteObj(texture), m_velocity{}, m_acceleration{} {
	}

	inline virtual void update(float dt) override {
		setPosition(position() + m_velocity * dt);
		m_velocity += m_acceleration * dt;
		StationarySpriteObj::update(dt);
	}

	GETTER(velocity, m_velocity)
	GETTER(acceleration, m_acceleration)

	SETTER(setVelocity, m_velocity)
	SETTER(setAcceleration, m_acceleration)
};