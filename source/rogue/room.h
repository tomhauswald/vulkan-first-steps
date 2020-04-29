#pragma once

#include "../engine.h"
#include "../model.h"
#include "../mesh_util.h"

class Room : public Model {
public:
	Room(Engine& e, size_t index) : 
		Model(
			e, 
			e.renderer().createMesh("room" + std::to_string(index)), 
			e.renderer().texture("wall")
		) {
		
		int walkableRadius = 6;
		int wallHeight = 3;
		
		auto vertices = std::vector<VPositionColorTexcoord>{};
		
		auto addCubeAt = [&vertices](glm::vec3 const& center) {
			for(auto const& v : createCubeVertices(center)) {
				vertices.push_back(v);
			}
		};

		for(int x=-walkableRadius; x<=walkableRadius; ++x) {
			for(int z=-walkableRadius; z<=walkableRadius; ++z) {
				if(x*x+z*z<=walkableRadius*walkableRadius) {
					addCubeAt({x, -0.5f, z });
					addCubeAt({x, wallHeight + 0.5f, z });
					if(x*x+z*z>(walkableRadius-1)*(walkableRadius-1)) {
						for(int y=0; y<wallHeight; ++y) {
							addCubeAt({x, y + 0.5f, z});
						}
					}
				}
			}
		}
		mesh().setVertices(vertices);
	}
};
