#include "renderer.h"
#include <glm/gtx/transform.hpp>
#include <chrono>
#include "camera.h"
#include "sprite.h"
#include "tilemap.h"
#include <unordered_set>

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
	Texture const* m_pDoorTexture;

public:
	Engine(EngineSettings const& settings) : 
		m_settings{ settings }, 
		m_renderer{ settings.renderer },
		m_objects{} {
		srand(time(nullptr));
	}

	/*void run3dTest() {
		m_renderer.initialize(false);

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

	static constexpr size_t floorTile = 30;
	static constexpr size_t lWallTile = 58;
	static constexpr size_t rWallTile = 57;
	static constexpr size_t hWallTile = 9;
	static constexpr size_t tlCornerTile = 59;
	static constexpr size_t trCornerTile = 60;
	static constexpr size_t blCornerTile = 73;
	static constexpr size_t brCornerTile = 74;

	void createRoom(Tilemap8& tilemap, glm::u64vec2 const& center, glm::u64vec2 const& size, std::vector<bool> const& floorMask) {

		auto pos = glm::u64vec2{};
		auto wallMask = std::vector<bool>(floorMask.size(), false);

		for (auto dx : range(size.x)) {
			pos.x = center.x - size.x / 2 + dx;
			for (auto dy : range(size.y)) {
				pos.y = center.y - size.y / 2 + dy;

				auto index = dx + dy * size.x;
				if (floorMask[index]) {

					if (dx > 0 && !floorMask[index - 1]) wallMask[index - 1] = true;
					if (dx < size.x - 1 && !floorMask[index + 1]) wallMask[index + 1] = true;
					if (dy > 0 && !floorMask[index - size.x]) wallMask[index - size.x] = true;
					if (dy < size.y - 1 && !floorMask[index + size.x]) wallMask[index + size.x] = true;

					if (dx > 0 && dy > 0 && !floorMask[index - 1 - size.x]) wallMask[index - 1 - size.x] = true;
					if (dx < size.x - 1 && dy > 0 && !floorMask[index + 1 - size.x]) wallMask[index + 1 - size.x] = true;
					if (dx > 0 && dy < size.y - 1 && !floorMask[index - 1 + size.x]) wallMask[index - 1 + size.x] = true;
					if (dx < size.x - 1 && dy < size.y - 1 && !floorMask[index + 1 + size.x]) wallMask[index + 1 + size.x] = true;

					tilemap.setTileAt({ pos.x,pos.y,0 }, floorTile);
				}
			}
		}

		std::vector<glm::u64vec2> tlcs = {};
		std::vector<glm::u64vec2> trcs = {};
		std::vector<glm::u64vec2> blcs = {};
		std::vector<glm::u64vec2> brcs = {};

		for (auto dx : range(size.x)) {
			pos.x = center.x - size.x / 2 + dx;
			for (auto dy : range(size.y)) {
				pos.y = center.y - size.y / 2 + dy;
				
				auto index = dx + dy * size.x;
				
				if (wallMask[index]) {
					if (dx < size.x - 1 &&
						dy < size.y - 1 &&
						wallMask[index + 1] &&
						wallMask[index + size.x]) {
						tlcs.push_back(pos);
						tilemap.setTileAt({ pos.x, pos.y, 0 }, tlCornerTile);
						tilemap.setTileAt({ pos.x, pos.y - 1, 1 }, tlCornerTile-7);
					}
					else if (
						dx > 0 &&
						dy < size.y - 1 &&
						wallMask[index - 1] &&
						wallMask[index + size.x]) {
						trcs.push_back(pos);
						tilemap.setTileAt({ pos.x, pos.y, 0 }, trCornerTile);
						tilemap.setTileAt({ pos.x, pos.y - 1, 1 }, trCornerTile-7);
					}
					else if (
						dx < size.x - 1 &&
						dy > 0 &&
						wallMask[index + 1] &&
						wallMask[index - size.x]) {
						blcs.push_back(pos);
						tilemap.setTileAt({ pos.x, pos.y, 0 }, blCornerTile);
						tilemap.setTileAt({ pos.x, pos.y - 1, 1 }, blCornerTile-7);
					}
					else if (
						dx > 0 &&
						dy > 0 &&
						wallMask[index - 1] &&
						wallMask[index - size.x]) {
						brcs.push_back(pos);
						tilemap.setTileAt({ pos.x, pos.y, 0 }, brCornerTile);
						tilemap.setTileAt({ pos.x, pos.y-1, 1 }, brCornerTile-7);
					}
				}
			}
		}

		glm::u64vec3 next;
		for (auto const& tlc : tlcs) {

			// Place horizontal walls to the right.
			next = { tlc.x + 1, tlc.y, 0 };
			while (!tilemap.tileAt(next)) {
				tilemap.setTileAt(next, hWallTile);
				tilemap.setTileAt({ next.x, next.y - 1, 1 }, 2);
				++next.x;
			}

			// Place left walls downwards.
			next = { tlc.x, tlc.y + 1, 0 };
			while (!tilemap.tileAt(next)) {
				tilemap.setTileAt(next, lWallTile);
				++next.y;
			}
		}

		for (auto const& trc : trcs) {
			
			// Place horizontal walls to the left.
			next = { trc.x - 1, trc.y, 0 };
			while (!tilemap.tileAt(next)) {
				tilemap.setTileAt(next, hWallTile);
				tilemap.setTileAt({ next.x, next.y - 1, 1 }, 2);
				--next.x;
			}

			// Place right walls downwards.
			next = { trc.x, trc.y + 1, 0 };
			while (!tilemap.tileAt(next)) {
				tilemap.setTileAt(next, rWallTile);
				++next.y;
			}
		}
		
		for (auto const& blc : blcs) {

			// Place horizontal walls to the right.
			next = { blc.x + 1, blc.y, 0 };
			while (!tilemap.tileAt(next)) {
				tilemap.setTileAt(next, hWallTile);
				tilemap.setTileAt({ next.x, next.y - 1, 1 }, 2);
				++next.x;
			}

			// Place left walls upwards.
			next = { blc.x, blc.y - 1, 0 };
			while (!tilemap.tileAt(next)) {
				tilemap.setTileAt(next, rWallTile);
				--next.y;
			}
		}
	}

	void run() {

		m_renderer.initialize(true);

		auto& counter = add(std::make_unique<FPSCounterObj>());

		auto& doorTexture = m_renderer.createTexture();
		doorTexture.updatePixelsWithImage("../assets/images/door.png");
		m_pDoorTexture = &doorTexture;

		auto& tileset = m_renderer.createTexture();
		tileset.updatePixelsWithImage("../assets/images/dungeon.png");
		
		auto& tilemap = add(std::make_unique<Tilemap8>(
			glm::u64vec3{ 1e4, 1e3, Sprite::numLayers }, 
			tileset, 
			glm::uvec2{ 16, 16 }, 
			glm::vec2{ 32, 32 }
		));

		auto columnHeights = std::vector<size_t>(20);
		for (size_t i = 0; i < columnHeights.size(); i += 2) {
			columnHeights[i] = columnHeights[i + 1] = 5ull + (rand() % 5ull);
		}

		createRoom(tilemap, { 10, 10 }, { 13, 8 }, {
			0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,1,1,1,1,0,0,0,1,1,1,1,0,
			0,1,1,1,1,1,1,1,1,1,1,1,0,
			0,1,1,1,1,1,0,0,0,1,1,1,0,
			0,1,1,1,1,1,0,0,0,1,1,1,0,
			0,0,0,1,1,1,0,0,0,1,1,1,0,
			0,0,0,1,1,1,1,1,1,1,1,1,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0
		});

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
	return 0;
}
