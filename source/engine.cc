#include "renderer.h"
#include <glm/gtx/transform.hpp>
#include <chrono>
#include "camera.h"
#include "sprite_objects.h"

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

struct FPSCounterObj : public GameObject {
private:
	size_t m_frames;
	float m_seconds;

public:
	FPSCounterObj() : m_frames{}, m_seconds{} { }
	virtual void update(float dt) override {
		m_seconds += dt;
		m_frames++;
		if(m_seconds >= 1.0f) {
			std::cout << static_cast<size_t>(m_frames / m_seconds) << lf;
			m_seconds = 0.0f;
			m_frames = 0;
		}
		GameObject::update(dt);
	}
	virtual ~FPSCounterObj() { }
};

class Engine {
private:
	EngineSettings const& m_settings;
	Renderer m_renderer;
	std::vector<std::unique_ptr<GameObject>> m_objects;

public:
	Engine(EngineSettings const& settings) : 
		m_settings{ settings }, 
		m_renderer{ settings.renderer },
		m_objects{} {
		srand(time(nullptr));
		add(std::make_unique<FPSCounterObj>());
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

	template<typename Obj>
	Obj& add(std::unique_ptr<Obj> obj) {
		auto& ref = *obj;
		m_objects.push_back(std::move(obj));
		return ref;
	}

	void run() {

		m_renderer.initialize();

		std::vector<Texture*> textures{};

		for (auto file : { "3", "6", "9", "12", "15", "18", "20", "22", "24" }) {
			textures.push_back(&m_renderer.createTexture());
			textures.back()->updatePixelsWithImage("../assets/images/"s + file + ".png"s);
		}

		auto const tilesize = 16;
		for(auto x : range(m_settings.renderer.resolution.x / tilesize)) {
			for(auto y : range(m_settings.renderer.resolution.y / tilesize)) {
				for(auto z : range(Sprite::numLayers)) {
					auto& sprite = add(std::make_unique<StationarySpriteObj>(*textures[rand() % textures.size()]));
					sprite.setPosition({x * tilesize, y * tilesize});
					sprite.setSize({tilesize, tilesize});
					sprite.setColor({ frand(0,1), frand(0,1), frand(0,1), frand(0,1) });
					sprite.setRotation(frand(0, 360));
					sprite.setLayer(z);
				}
			}
		}

		auto prev = std::chrono::high_resolution_clock::now(); 
		
		while (m_renderer.isWindowOpen()) {

			auto now = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - prev).count();
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
};

int main() { 
	Engine({
		.renderer = {
			.windowTitle = "Vulkan Renderer",
			.resolution = {1600, 900},
			.vsyncEnabled = false
		}
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	(void) getchar();
	
	return 0;
}
