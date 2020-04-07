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

struct Sprite {

	Rectf bounds;
	Texture const* pTexture;
	Rectf textureArea;
	glm::vec3 color;
	float drawOrder;

	inline void setPosition(glm::vec2 const& pos) noexcept { 
		bounds.x = pos.x;
		bounds.y = pos.y; 
	}

	inline void setExtent(glm::vec2 const& ext) noexcept {
		bounds.w = ext.x;
		bounds.h = ext.y;
	}
};