#pragma once

#include "../engine.h"
#include "../model.h"
#include "../mesh_util.h"

class Room : public Model {
private:
	size_t m_index;
	glm::uvec3 m_origin;
	glm::uvec3 m_size;

public:
	Room(Engine& e, size_t index, glm::uvec3 origin, glm::uvec3 size) :
		Model(e, e.renderer().createMesh("room" + std::to_string(index)),
			e.renderer().texture("stonebrick_mossy")),
		m_index(index),
		m_origin(std::move(origin)),
		m_size(std::move(size)) {

		auto vertices = std::vector<VPositionColorTexcoord>{};

		const auto addCubeAt = [&vertices](glm::vec3 const& pos, glm::vec3 const& size, glm::vec3 const& col = { 1,1,1 }) {
			for (auto const& v : cubeVertices(pos + size / 2.0f, size, col, size)) {
				vertices.push_back(v);
			}
		};

		// floor
		addCubeAt({ 0,0,0 }, { size.x, 1, size.z });

		// ceiling
		addCubeAt({ 0,size.y - 1,0 }, { size.x, 1, size.z });

		// south wall
		addCubeAt({ 0,1,0 }, { size.x, size.y - 2, 1 }); // , { 1.0f,1.0f,0.7f });

		// north wall
		addCubeAt({ 0,1,size.z - 1 }, { (size.x - 3) / 2.0f, size.y - 2, 1 }); // , { 0.7f,0.7f,1.0f });
		addCubeAt({ size.x - (size.x - 3) / 2.0f,1,size.z - 1 }, { (size.x - 3) / 2.0f, size.y - 2, 1 }); // , { 0.7f,0.7f,1.0f });

		// west wall
		addCubeAt({ 0,1,1 }, { 1, size.y - 2, size.z - 2 }); // , { 0.7f,1.0f,1.0f });

		// east wall
		addCubeAt({ size.x - 1,1,1 }, { 1, size.y - 2, size.z - 2 }); // , { 1.0f,0.7f,0.7f });

		mesh().setVertices(vertices);
	}
};
