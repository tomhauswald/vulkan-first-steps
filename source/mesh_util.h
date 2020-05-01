#pragma once

#include "mesh.h"

inline auto cubeVertices(
	glm::vec3 const& center = { 0,0,0 },
	glm::vec3 const& size = { 1,1,1 },
	glm::vec3 const& color = { 1,1,1 },
	glm::vec3 const& uvScale = { 1,1,1 }
) {
	auto vertices = std::vector<VPositionColorTexcoord>{

		// south
		{ {-1, +1, -1}, color, {0,0} },
		{ {+1, +1, -1}, color, {uvScale.x, 0} },
		{ {+1, -1, -1}, color, {uvScale.x, uvScale.y} },
		{ {+1, -1, -1}, color, {uvScale.x, uvScale.y} },
		{ {-1, -1, -1}, color, {0, uvScale.y} },
		{ {-1, +1, -1}, color, {0,0} },

		// down
		{ {-1, -1, -1}, color, {0,0} },
		{ {+1, -1, -1}, color, {uvScale.x,0} },
		{ {+1, -1, +1}, color, {uvScale.x,uvScale.z} },
		{ {+1, -1, +1}, color, {uvScale.x,uvScale.z} },
		{ {-1, -1, +1}, color, {0,uvScale.z} },
		{ {-1, -1, -1}, color, {0,0} },

		// west
		{ {-1, +1, +1}, color, {0,0} },
		{ {-1, +1, -1}, color, {uvScale.z,0} },
		{ {-1, -1, -1}, color, {uvScale.z,uvScale.y} },
		{ {-1, -1, -1}, color, {uvScale.z,uvScale.y} },
		{ {-1, -1, +1}, color, {0,uvScale.y} },
		{ {-1, +1, +1}, color, {0,0} },

		// east
		{ {+1, +1, -1}, color, {0,0} },
		{ {+1, +1, +1}, color, {uvScale.z,0} },
		{ {+1, -1, +1}, color, {uvScale.z,uvScale.y} },
		{ {+1, -1, +1}, color, {uvScale.z,uvScale.y} },
		{ {+1, -1, -1}, color, {0,uvScale.y} },
		{ {+1, +1, -1}, color, {0,0} },

		// north
		{ {+1, +1, +1}, color, {0,0} },
		{ {-1, +1, +1}, color, {uvScale.x,0} },
		{ {-1, -1, +1}, color, {uvScale.x,uvScale.y} },
		{ {-1, -1, +1}, color, {uvScale.x,uvScale.y} },
		{ {+1, -1, +1}, color, {0,uvScale.y} },
		{ {+1, +1, +1}, color, {0,0} },

		// up
		{ {-1, +1, +1}, color, {0,0} },
		{ {+1, +1, +1}, color, {uvScale.x,0} },
		{ {+1, +1, -1}, color, {uvScale.x,uvScale.z} },
		{ {+1, +1, -1}, color, {uvScale.x,uvScale.z} },
		{ {-1, +1, -1}, color, {0,uvScale.z} },
		{ {-1, +1, +1}, color, {0,0} }
	};

	for (auto& v : vertices) v.position = center + 0.5f * size * v.position;

	return vertices;
}

