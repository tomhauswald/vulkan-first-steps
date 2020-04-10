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
	Texture const* m_pTexture;

	// Source texture coordinates.
	glm::vec4 m_textureArea;

	// Color multiplier.
	glm::vec4 m_color;

	// Clockwise rotation in degrees.
	float m_rotation; 

	// Sprites with lower draw order are drawn
	// before (i.e. below) those with higher draw order.
	float m_drawOrder;

public:
	GETTER(position, m_position)
	GETTER(size, m_size)
	GETTER(texture, m_pTexture)
	GETTER(textureArea, m_textureArea)
	GETTER(color, m_color)
	GETTER(rotation, m_rotation)
	GETTER(drawOrder, m_drawOrder)

	SETTER(setPosition, m_position)
	SETTER(setSize, m_size)
	SETTER(setTexture, m_pTexture)
	SETTER(setTextureArea, m_textureArea)
	SETTER(setColor, m_color)
	SETTER(setRotation, m_rotation)
	SETTER(setDrawOrder, m_drawOrder)
};