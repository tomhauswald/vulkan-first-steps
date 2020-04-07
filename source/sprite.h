#pragma once

#include "common.h"
#include "shader_interface.h"
#include "texture.h"

struct Rectf {
	float x;
	float y;
	float w;
	float h;
};

struct Boxf {
	float x;
	float y;
	float z;
	float w;
	float h;
	float d;
};

class Sprite {
private:
	Rectf m_bounds;
	Rectf m_textureArea;
	float m_drawOrder;
	Texture const& m_texture;

public:
	inline Sprite(Texture const& texture) :
		m_bounds{ 
			0.0f,
			0.0f,
			static_cast<float>(texture.width()), 
			static_cast<float>(texture.height()) 
		},
		m_textureArea{ 0,0,1,1 },
		m_drawOrder{ 0 },
		m_texture{ texture } {
	}

	GETTER(bounds, m_bounds)
	GETTER(textureArea, m_textureArea)
	GETTER(drawOrder, m_drawOrder)
	GETTER(texture, m_texture)

	SETTER(setBounds, m_bounds)
	SETTER(setTextureArea, m_textureArea)
	SETTER(setDrawOrder, m_drawOrder)

	inline void setPosition(glm::vec2 const& pos) noexcept { 
		m_bounds.x = pos.x;
		m_bounds.y = pos.y; 
	}

	inline void setExtent(glm::vec2 const& ext) noexcept {
		m_bounds.w = ext.x;
		m_bounds.h = ext.y;
	}
};