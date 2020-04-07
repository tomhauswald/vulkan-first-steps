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
	Texture const* m_pTexture;

public:
	inline Sprite() :
		m_bounds{ 0,0,32,32 },
		m_textureArea{ 0,0,1,1 },
		m_drawOrder{ 0 },
		m_pTexture{ nullptr } {
	}

	inline Sprite(Texture const& texture) :
		m_bounds{ 
			0, 
			0, 
			static_cast<float>(texture.width()), 
			static_cast<float>(texture.height()) 
		},
		m_textureArea{ 0,0,1,1 },
		m_drawOrder{ 0 },
		m_pTexture{ &texture } {
	}

	inline Rectf const& bounds() const noexcept { 
		return m_bounds; 
	}

	inline Rectf const& textureArea() const noexcept { 
		return m_textureArea;
	}

	inline float drawOrder() const noexcept { 
		return m_drawOrder; 
	}

	inline Texture const* texture() const noexcept { 
		return m_pTexture; 
	}

	/*
	inline void setPosition(glm::vec2 const& pos) noexcept { 
		m_bounds.x = pos.x; 
		m_bounds.y = pos.y; 
	}

	inline void setExtent(glm::vec2 const& ext) noexcept {
		m_bounds.w = ext.x;
		m_bounds.h = ext.y;
	}
	*/

	inline void setBounds(Rectf bounds) noexcept { 
		m_bounds = std::move(bounds); 
	}

	inline void setTextureArea(Rectf area) noexcept {
		m_textureArea = std::move(area); 
	}

	inline void setDrawOrder(float order) noexcept {
		m_drawOrder = order; 
	}

	inline void setTexture(Texture const& texture) noexcept { 
		m_pTexture = &texture; 
	}
};