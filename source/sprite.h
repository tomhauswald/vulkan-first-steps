#pragma once

#include "game_object.h"
#include "texture.h"

class Sprite : public GameObject {
private:
	glm::vec2 m_position;
	glm::vec2 m_size;
	glm::vec4 m_textureArea;
	glm::vec4 m_color;
	float m_rotation;
	uint8_t m_layer;
	Texture const& m_texture;

public:
	static constexpr uint8_t numLayers = 4;

	inline Sprite(Engine& e, Texture const& texture) :
		GameObject{ e },
		m_position{ },
		m_size{texture.width(), texture.height()},
		m_textureArea{ 0,0,1,1 },
		m_color{ 1,1,1,1 },
		m_rotation{ 0 },
		m_layer{ 0 },
		m_texture{ texture } {
	}
	
	inline virtual ~Sprite() {
	}
	
	inline virtual void update(float dt) override {
		GameObject::update(dt);
	}

	virtual void draw(Renderer& r) const override;
	
	inline void setLayer(uint8_t layer) {
		crashIf(layer >= numLayers);
		m_layer = layer;
	}

	GETTER(position, m_position)
	GETTER(size, m_size)
	GETTER(texture, m_texture)
	GETTER(textureArea, m_textureArea)
	GETTER(color, m_color)
	GETTER(rotation, m_rotation)
	GETTER(layer, m_layer)

	SETTER(setPosition, m_position)
	SETTER(setSize, m_size)
	SETTER(setTextureArea, m_textureArea)
	SETTER(setColor, m_color)
	SETTER(setRotation, m_rotation)
};

class KinematicSprite : public Sprite {
private:
	glm::vec2 m_velocity;
	glm::vec2 m_acceleration;

public:
	inline KinematicSprite(Engine& e, Texture const& texture) :
		Sprite(e, texture), m_velocity{}, m_acceleration{} {
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
