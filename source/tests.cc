#include "engine.h"
#include "model.h"

static void makeCube(Mesh& mesh) {
	
	mesh.setVertices({
			
		{ {-1, +1, -1}, {1,1,0}, {0,0} },
		{ {+1, +1, -1}, {1,1,0}, {1,0} },
		{ {+1, -1, -1}, {1,1,0}, {1,1} },
		{ {+1, -1, -1}, {1,1,0}, {1,1} },
		{ {-1, -1, -1}, {1,1,0}, {0,1} },
		{ {-1, +1, -1}, {1,1,0}, {0,0} },

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
	});
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
	
	auto& texture = engine.renderer().createTexture();
	texture.updatePixelsWithImage("../assets/images/wall.png");

	for(auto i : range(10000)) { (void) i;
		auto& sprite = engine.add(std::make_unique<Sprite>(texture));
		sprite.setPosition({ rand() % 1600, rand() % 900 });
		sprite.setSize({ 100 + rand() % 100, 100 + rand() % 100 });
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

	auto& texture = engine.renderer().createTexture();
	texture.updatePixelsWithImage("../assets/images/wall.png");
	
	auto& cube = engine.renderer().createMesh();
	makeCube(cube);
	
	for(int x=-1; x<=1; ++x) {
		for(int y=-1; y<=1; ++y) {
			for(int z=-1; z<=1; ++z) {
				auto& model = engine.add(std::make_unique<Model>(cube, texture));
				model.setPosition(3.0f * glm::vec3{ x, y, z });
				model.setEuler({x * 180, y * 180, z * 180});
			}
		}
	}

	auto cam = engine.renderer().camera3d();
	cam.setPosition({ 0.0f, 0.0f, -10.0f });
	cam.lookAt({ 0.0f, 0.0f, 0.0f });
	engine.renderer().setCamera3d(cam);

	engine.run();
}

int main() { 
	t00_sprites();
	t01_models();
	return 0;
}

