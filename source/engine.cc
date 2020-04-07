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

				r.renderMesh(cube, { modelMatrix });
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
	}

	void run() {
		m_renderer.initialize();

		auto const& cube = createCubeMesh(m_renderer);
		
		auto& texture = m_renderer.createTexture();
		texture.updatePixelsWithImage("../assets/images/coffee.png");
		auto sprite = Sprite(texture);

#ifdef D3
		auto camera = Camera3d(
			m_settings.renderer.resolution.x / (float)m_settings.renderer.resolution.y,
			45.0f
		);
		camera.setPosition({ 0.0f, 0.0f, -10.0f });
		camera.lookAt({ 0.0f, 0.0f, 0.0f });
#else
		auto camera = Camera2d({
			m_settings.renderer.resolution.x,
			m_settings.renderer.resolution.y
		});
#endif

		while (m_renderer.isWindowOpen()) {

			if (m_renderer.tryBeginFrame()) {

				m_renderer.setCameraTransform(camera.transform());

#ifdef D3
				m_renderer.bindTexture(texture);
				renderCubes(m_renderer, cube);
#else
				m_renderer.renderSprite(sprite);
#endif

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
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	(void) getchar();
	
	return 0;
}
