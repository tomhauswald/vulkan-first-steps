#include "renderer.h"
#include <glm/gtx/transform.hpp>
#include <chrono>
#include "camera.h"

struct EngineSettings {
	RendererSettings renderer;
};

Mesh const* cubeMesh;

void createCubeMesh(Renderer& r) {

	auto& cube = r.createMesh();
	cube.setVertices({

		{ {+1, +1, -1}, {1,1,0} },
		{ {+1, -1, -1}, {1,1,0} },
		{ {-1, +1, -1}, {1,1,0} },
		{ {+1, -1, -1}, {1,1,0} },
		{ {-1, -1, -1}, {1,1,0} },
		{ {-1, +1, -1}, {1,1,0} },

		{ {-1, +1, +1}, {0,1,1} },
		{ {-1, +1, -1}, {0,1,1} },
		{ {-1, -1, -1}, {0,1,1} },
		{ {-1, -1, -1}, {0,1,1} },
		{ {-1, -1, +1}, {0,1,1} },
		{ {-1, +1, +1}, {0,1,1} },

		{ {+1, +1, -1}, {1,0,0} },
		{ {+1, +1, +1}, {1,0,0} },
		{ {+1, -1, +1}, {1,0,0} },
		{ {+1, -1, +1}, {1,0,0} },
		{ {+1, -1, -1}, {1,0,0} },
		{ {+1, +1, -1}, {1,0,0} },

		{ {+1, +1, +1}, {0,0,1} },
		{ {-1, +1, +1}, {0,0,1} },
		{ {-1, -1, +1}, {0,0,1} },
		{ {-1, -1, +1}, {0,0,1} },
		{ {+1, -1, +1}, {0,0,1} },
		{ {+1, +1, +1}, {0,0,1} },

		{ {-1, +1, +1}, {0,1,0} },
		{ {+1, +1, +1}, {0,1,0} },
		{ {+1, +1, -1}, {0,1,0} },
		{ {+1, +1, -1}, {0,1,0} },
		{ {-1, +1, -1}, {0,1,0} },
		{ {-1, +1, +1}, {0,1,0} },

		{ {-1, -1, -1}, {1,0,1} },
		{ {+1, -1, -1}, {1,0,1} },
		{ {+1, -1, +1}, {1,0,1} },
		{ {+1, -1, +1}, {1,0,1} },
		{ {-1, -1, +1}, {1,0,1} },
		{ {-1, -1, -1}, {1,0,1} }
	});

	cubeMesh = &cube;
}

void renderCubes(Renderer& r) {

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

				r.renderMesh(*cubeMesh, { modelMatrix });
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

		createCubeMesh(m_renderer);

		auto camera = Camera(
			m_settings.renderer.resolution.x / (float)m_settings.renderer.resolution.y,
			45.0f
		);
		camera.setPosition({ 0.0f, 0.0f, -10.0f });
		camera.lookAt({ 0.0f, 0.0f, 0.0f });

		while (m_renderer.isWindowOpen()) {

			if (m_renderer.tryBeginFrame()) {

				auto uniformData = UniformData{};
				uniformData.viewMatrix = camera.viewMatrix();
				uniformData.projectionMatrix = camera.projMatrix();
				m_renderer.setUniformData(uniformData);

				renderCubes(m_renderer);

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
