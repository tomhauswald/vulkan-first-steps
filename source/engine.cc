#include "renderer.h"
#include "camera.h"
#include "sprite.h"
#include "tilemap.h"

#include <glm/gtx/transform.hpp>
#include <chrono>
#include <unordered_set>

struct EngineSettings {
	RendererSettings renderer;
};

struct CubeDemo : public GameObject {
private:
	static Mesh const& createCubeMesh(Renderer& r) {
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

	Mesh const& m_mesh;
	Texture& m_texture;

public:
	CubeDemo(Renderer& r) : 
		m_mesh{createCubeMesh(r)},
		m_texture{r.createTexture()} {
		m_texture.updatePixelsWithImage("../assets/images/door.png");
	} 

	void draw(Renderer& r) const override {
		
		r.bindTextureSlot(0, m_texture);

		for (int z = -1; z <= 1; ++z) {
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) {

					auto modelMatrix = glm::translate(
						glm::vec3{ x * 3.0f, y * 3.0f, z * 3.0f }
					);

					if (x || y || z) {
						modelMatrix = glm::rotate(
							modelMatrix,
							lifetime(),
							{ x, y, z }
						);
					}

					r.setPushConstants(PCInstanceTransform{ modelMatrix });
					r.renderMesh(m_mesh);
				}
			}
		}
	}
};

class Engine {
private:
	EngineSettings const& m_settings;
	Renderer m_renderer;
	std::vector<std::unique_ptr<GameObject>> m_objects;
	float m_totalTime;

public:
	Engine(EngineSettings const& settings) : 
		m_settings{ settings }, 
		m_renderer{ settings.renderer },
		m_objects{},
		m_totalTime{} {
		srand(time(nullptr));
	}

	template<typename Obj>
	Obj& add(std::unique_ptr<Obj> obj) {
		auto& ref = *obj;
		m_objects.push_back(std::move(obj));
		return ref;
	}

	void mainLoop() {

		auto prev = std::chrono::high_resolution_clock::now(); 

		while (m_renderer.isWindowOpen()) {
			auto now = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - prev).count();
			m_totalTime += dt;
			prev = now;

			std::remove_if(m_objects.begin(), m_objects.end(), [](std::unique_ptr<GameObject> const& pObj) {
				return !pObj->alive();
			});

			if (m_renderer.tryBeginFrame()) {
				for (auto& object : m_objects) {
					object->update(dt);
					object->draw(m_renderer);
				}
				m_renderer.endFrame();
			}
			m_renderer.handleWindowEvents();
		}
	}

	void run2dTest() {
		m_renderer.initialize(true);
		mainLoop();		
	}

	void run3dTest() {
		m_renderer.initialize(false);
		//add(std::make_unique<CubeDemo>(m_renderer));

		auto cam = m_renderer.camera3d();
		cam.setPosition({ 0.0f, 0.0f, -10.0f });
		cam.lookAt({ 0.0f, 0.0f, 0.0f });
		m_renderer.setCamera3d(cam);

		mainLoop();
	}
};

int main() { 
	Engine({
		.renderer = {
			.windowTitle = "Vulkan Renderer",
			.resolution = {1600, 900},
			.vsyncEnabled = false
		}
	}).run3dTest();
	return 0;
}
