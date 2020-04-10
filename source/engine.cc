#include "renderer.h"
#include <glm/gtx/transform.hpp>
#include <chrono>
#include "camera.h"

struct EngineSettings {
	RendererSettings renderer;
};

Mesh const& createCubeMesh(Renderer& r) {

	auto& cube = r.createMesh();
	cube.setVertices({
		
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
		{ {-1, +1, +1}, {0,1,0}, {0,0} },

		{ {-1, -1, -1}, {1,0,1}, {0,0} },
		{ {+1, -1, -1}, {1,0,1}, {1,0} },
		{ {+1, -1, +1}, {1,0,1}, {1,1} },
		{ {+1, -1, +1}, {1,0,1}, {1,1} },
		{ {-1, -1, +1}, {1,0,1}, {0,1} },
		{ {-1, -1, -1}, {1,0,1}, {0,0} }
	});

	return cube;
}

void renderCubes(Renderer& r, Mesh const& cube) {

	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {

				auto modelMatrix = glm::translate(
					glm::vec3{ x * 3.0f, y * 3.0f, z * 3.0f }
				);

				if (x || y || z) {
					modelMatrix *= glm::rotate(
						glm::mat4{ 1.0f },
						std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
						{ x, y, z }
					);
				}

				r.setPushConstants(PCInstanceTransform{ modelMatrix });
				r.renderMesh(cube);
			}
		}
	}
}

class Engine {
private:
	[[maybe_unused]] EngineSettings const& m_settings;
	Renderer m_renderer;

public:
	Engine(EngineSettings const& settings) : 
		m_settings{ settings }, 
		m_renderer{ settings.renderer } {
		srand(time(nullptr));
	}

	/*void run3dTest() {
		m_renderer.initialize();

		auto width = static_cast<float>(m_settings.renderer.resolution.x);
		auto height = static_cast<float>(m_settings.renderer.resolution.y);

		auto const& cube = createCubeMesh(m_renderer);

		auto cam3d = Camera3d(width / height, 45.0f);
		cam3d.setPosition({ 0.0f, 0.0f, -10.0f });
		cam3d.lookAt({ 0.0f, 0.0f, 0.0f });

		while (m_renderer.isWindowOpen()) {
			if (m_renderer.tryBeginFrame()) {
				renderCubes(m_renderer, cube);
				m_renderer.endFrame();
			}
			m_renderer.handleWindowEvents();
		}
	}*/

	std::vector<Sprite> createRandomSprites(Texture const& texture, size_t count) {

		auto sprites = std::vector<Sprite>(count, { texture });

		auto width = static_cast<float>(m_settings.renderer.resolution.x);
		auto height = static_cast<float>(m_settings.renderer.resolution.y);

		for (auto& sprite : sprites) {
			sprite.setSize({ frand(64, 256), frand(64, 256) });
			sprite.setPosition({
				frand(-sprite.size().x, width),
				frand(-sprite.size().y, height)
			});
			sprite.setColor({ frand(0,1), frand(0,1), frand(0,1), frand(0,1) });
			sprite.setRotation(frand(0, 360));
			sprite.setDrawOrder(frand(-1, 1));
		}

		return sprites;
	}

	void run2dTest() {

		m_renderer.initialize();

		auto& texture = m_renderer.createTexture();
		texture.updatePixelsWithImage("../assets/images/3.png");
	
		auto cam2d = Camera2d({
			m_settings.renderer.resolution.x,
			m_settings.renderer.resolution.y
		});

		auto sprites = createRandomSprites(texture, 54000);

		while (m_renderer.isWindowOpen()) {
			if (m_renderer.tryBeginFrame()) {
				for (auto const& sprite : sprites) {
					m_renderer.renderSprite(sprite, cam2d);
				}
				m_renderer.endFrame();
			}
			m_renderer.handleWindowEvents();
		}
	}
};

int main() { 
	Engine({
		.renderer = {
			.windowTitle = "Vulkan Renderer",
			.resolution = {1440, 900},
			.vsyncEnabled = false
		}
	}).run2dTest();

	std::cout << "Press [ENTER] exit application..." << lf;
	(void) getchar();
	
	return 0;
}
