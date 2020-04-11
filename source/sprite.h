#pragma once

#include "common.h"
#include "shader_interface.h"
#include "texture.h"

class Sprite {
private:
	// Screen space (pixel) coordinates.
	glm::vec2 m_position;

	// Pixel width and height.
	glm::vec2 m_size;

	// Source texture.
	Texture const& m_texture;

	// Source texture coordinates.
	glm::vec4 m_textureArea;

	// Color multiplier.
	glm::vec4 m_color;

	// Clockwise rotation in degrees.
	float m_rotation; 

	// Sprites with higher layer number are drawn over
	// sprites with lower layer numbers.
	uint8_t m_layer;

public:
	static constexpr uint8_t numLayers = 4;

	inline Sprite(Texture const& texture) :
		m_position{},
		m_size{texture.width(), texture.height()},
		m_texture{ texture },
		m_textureArea{ 0,0,1,1 },
		m_color{ 1,1,1,1 },
		m_rotation{ 0 },
		m_layer{ 0 } {
	}

	inline void setLayer(uint8_t layer) noexcept {
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