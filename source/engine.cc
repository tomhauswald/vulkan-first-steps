#include "renderer.h"
#include "camera.h"
#include "sprite.h"
#include "tilemap.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <chrono>
#include <unordered_set>

struct EngineSettings {
	RendererSettings renderer;
};

class Model : public GameObject {
private:
	Mesh& m_mesh;
	Texture const& m_texture;
	glm::vec3 m_position;
	glm::vec3 m_scale;
	glm::vec3 m_euler;

public:
	Model(Mesh& mesh, Texture const& texture) : 
		m_mesh{ mesh },
		m_texture{ texture },
		m_position{ 0, 0, 0 },
		m_scale{ 1, 1, 1 },
		m_euler{ 0, 0, 0 } {
	}
	
	void draw(Renderer& r) const override {
		r.bindTextureSlot(0, m_texture);
		r.setPushConstants(PCInstanceTransform{
			glm::translate(m_position) 
			* glm::scale(m_scale)
			* glm::eulerAngleYXZ(m_euler.y, m_euler.x, m_euler.z)
		});
		r.renderMesh(m_mesh);
	}

	GETTER(mesh, m_mesh)
	GETTER(position, m_position)
	GETTER(scale, m_scale)
	GETTER(euler, m_euler)

	SETTER(setPosition, m_position)
	SETTER(setScale, m_scale)
	SETTER(setEuler, m_euler)
};

class CubeDemo : public GameObject {
private:
	static Mesh& createCubeMesh(Renderer& r) {
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

	std::unique_ptr<Model> m_pModel;

public:
	CubeDemo(Renderer& r) {
		auto& mesh = createCubeMesh(r);
		auto& texture = r.createTexture();
		texture.updatePixelsWithImage("../assets/images/wall.png");
		m_pModel = std::make_unique<Model>(mesh, texture);
	} 

	void draw(Renderer& r) const override {
		
		for (int z = -1; z <= 1; ++z) {
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) {
					m_pModel->setPosition({ x * 3.0f, y * 3.0f, z * 3.0f });
					m_pModel->setEuler({ !!x * lifetime(), !!y * lifetime(), !!z * lifetime() });
					m_pModel->draw(r);
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
		add(std::make_unique<CubeDemo>(m_renderer));

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
