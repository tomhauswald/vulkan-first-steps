#pragma once

#include "engine.h"
#include "model.h"

static std::vector<VPositionColorTexcoord> createCubeVertices(glm::vec3 const& center) {
	
	auto vertices = std::vector<VPositionColorTexcoord> {
			
		{ {-1, +1, -1}, {1,1,0}, {0,0} },
		{ {+1, +1, -1}, {1,1,0}, {1,0} },
		{ {+1, -1, -1}, {1,1,0}, {1,1} },
		{ {+1, -1, -1}, {1,1,0}, {1,1} },
		{ {-1, -1, -1}, {1,1,0}, {0,1} },
		{ {-1, +1, -1}, {1,1,0}, {0,0} },
		
		{ {-1, -1, -1}, {1,0,1}, {0,0} },
		{ {+1, -1, -1}, {1,0,1}, {1,0} },
		{ {+1, -1, +1}, {1,0,1}, {1,1} },
		{ {+1, -1, +1}, {1,0,1}, {1,1} },
		{ {-1, -1, +1}, {1,0,1}, {0,1} },
		{ {-1, -1, -1}, {1,0,1}, {0,0} },

		{ {-1, +1, +1}, {0,1,1}, {0,0} },
		{ {-1, +1, -1}, {0,1,1}, {1,0} },
		{ {-1, -1, -1}, {0,1,1}, {1,1} },
		{ {-1, -1, -1}, {0,1,1}, {1,1} },
		{ {-1, -1, +1}, {0,1,1}, {0,1} },
		{ {-1, +1, +1}, {0,1,1}, {0,0} },

		{ {+1, +1, -1}, {1,0,0}, {0,0} },
		{ {+1, +1, +1}, {1,0,0}, {1,0} },
		{ {+1, -1, +1}, {1,0,0}, {1,1} },
		{ {+1, -1, +1}, {1,0,0}, {1,1} },
		{ {+1, -1, -1}, {1,0,0}, {0,1} },
		{ {+1, +1, -1}, {1,0,0}, {0,0} },

		{ {+1, +1, +1}, {0,0,1}, {0,0} },
		{ {-1, +1, +1}, {0,0,1}, {1,0} },
		{ {-1, -1, +1}, {0,0,1}, {1,1} },
		{ {-1, -1, +1}, {0,0,1}, {1,1} },
		{ {+1, -1, +1}, {0,0,1}, {0,1} },
		{ {+1, +1, +1}, {0,0,1}, {0,0} },

		{ {-1, +1, +1}, {0,1,0}, {0,0} },
		{ {+1, +1, +1}, {0,1,0}, {1,0} },
		{ {+1, +1, -1}, {0,1,0}, {1,1} },
		{ {+1, +1, -1}, {0,1,0}, {1,1} },
		{ {-1, +1, -1}, {0,1,0}, {0,1} },
		{ {-1, +1, +1}, {0,1,0}, {0,0} }
	};

	for(auto& v : vertices) v.position = center + 0.5f * v.position;
	return vertices;
}

void t00_sprites() {
	auto engine = Engine({
		.renderer = {
			.windowTitle = "Rendering 10K Sprites",
			.resolution = {1600, 900},
			.enableVsync = false,
			.enable2d = true
		}
	});
	
	engine.renderer().createTexture("wall").updatePixelsWithImage("../assets/images/wall.png");

	for(auto i : range(10000)) { (void) i;
		auto& sprite = engine.add<Sprite>(engine.renderer().texture("wall"));
		sprite.setPosition({ rand() % 1600, rand() % 900 });
		sprite.setSize({ 16 + rand() % 64, 16 + rand() % 64 });
		sprite.setRotation(rand() % 360);
	}	
	
	engine.run();
}

void t01_models() {
	auto engine = Engine({
		.renderer = {
			.windowTitle = "Rendering 27 Cube Models",
			.resolution = {1600, 900},
			.enableVsync = false,
			.enable2d = false
		}
	});

	engine.renderer().createTexture("wall").updatePixelsWithImage("../assets/images/wall.png");
	engine.renderer().createMesh("cube").setVertices(createCubeVertices({}));
	
	for(int x=-1; x<=1; ++x) {
		for(int y=-1; y<=1; ++y) {
			for(int z=-1; z<=1; ++z) {
				auto& model = engine.add<Model>(
					engine.renderer().mesh("cube"), 
					engine.renderer().texture("wall")
				);
				model.setPosition(3.0f * glm::vec3{ x, y, z });
				model.setEuler({x * 180, y * 180, z * 180});
			}
		}
	}

	engine.run();
}
