#include "renderer.h"
#include <glm/gtx/transform.hpp>
#include <chrono>
#include "camera.h"
#include "sprite.h"
#include "tilemap.h"

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

	static constexpr auto floorTiles = std::array {
		30, 30, 30, 30, 30, 30, 30, 30,
		30, 30, 30, 30, 30, 30, 30, 30,
		30, 30, 30, 30, 30, 30, 30, 30,
		32, 32, 32, 32, 38, 38, 38, 38,
		31, 31, 31, 31, 37, 39, 44, 45
	};

	static constexpr auto wallTopTiles = std::array {
		2, 3, 4, 75, 76
	};

	static constexpr size_t wallFrontTile = 10;
	static constexpr size_t wallLeftTile = 58;
	static constexpr size_t wallRightTile = 57;

	static constexpr size_t wallTLCornerTopTile = 52;
	static constexpr size_t wallTLCornerFrontTile = 59;

	static constexpr size_t wallTRCornerTopTile = 53;
	static constexpr size_t wallTRCornerFrontTile = 60;

	static constexpr size_t wallBLCornerTopTile = 66;
	static constexpr size_t wallBLCornerFrontTile = 73;

	static constexpr size_t wallBRCornerTopTile = 67;
	static constexpr size_t wallBRCornerFrontTile = 74;

	void createCornerAboveLeft(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y - 2, 1 }, wallTLCornerTopTile);
		tilemap.setTileAt({ pos.x, pos.y - 1, 0 }, wallTLCornerFrontTile);
	}

	void createCornerAboveRight(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y - 2, 1 }, wallTRCornerTopTile);
		tilemap.setTileAt({ pos.x, pos.y - 1, 0 }, wallTRCornerFrontTile);
	}

	void createCornerBelowLeft(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y, 1 }, wallBLCornerTopTile);
		tilemap.setTileAt({ pos.x, pos.y + 1, 0 }, wallBLCornerFrontTile);
	}

	void createCornerBelowRight(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y, 1 }, wallBRCornerTopTile);
		tilemap.setTileAt({ pos.x, pos.y + 1, 0 }, wallBRCornerFrontTile);
	}

	void createWallAbove(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y - 2, 1 }, randomPick(wallTopTiles));
		tilemap.setTileAt({ pos.x, pos.y - 1, 0 }, wallFrontTile);
	}

	void createWallBelow(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y, 1 }, randomPick(wallTopTiles));
		tilemap.setTileAt({ pos.x, pos.y + 1, 0 }, wallFrontTile);
	}

	void createWallToLeft(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y, 1 }, wallLeftTile);
	}

	void createWallToRight(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y, 1 }, wallRightTile);
	}

	void createFloor(Tilemap8& tilemap, glm::u64vec2 const& pos) {
		tilemap.setTileAt({ pos.x, pos.y, 0 }, randomPick(floorTiles));
	}

	void createRoom(Tilemap8& tilemap, glm::u64vec2 const& center, glm::u64vec2 const& size, std::vector<bool> const& mask) {

		auto pos = glm::u64vec2{};

		for (auto dx : range(size.x)) {

			pos.x = center.x + dx;

			for (auto dy : range(size.y)) {

				pos.y = center.y + dy;
				auto index = dx + dy * size.x;

				auto canStand     = mask[index];
				auto canMoveLeft  = (dx > 0          && mask[index - 1]);
				auto canMoveRight = (dx < size.x - 1 && mask[index + 1]);
				auto canMoveUp    = (dy > 0          && mask[index - size.x]);
				auto canMoveDown  = (dy < size.y - 1 && mask[index + size.x]);

				if (!canStand) continue;
				
				createFloor(tilemap, pos);

				if (!canMoveUp) createWallAbove(tilemap, pos);

				if (!canMoveDown) createWallBelow(tilemap, pos);

				if (!canMoveLeft) {
					if(canMoveUp && canMoveDown) createWallToLeft(tilemap, pos);
					if (!canMoveUp) createCornerAboveLeft(tilemap, pos);
					if (!canMoveDown) createCornerBelowLeft(tilemap, pos);
				}

				if (!canMoveRight) {
					if (canMoveUp && canMoveDown) createWallToRight(tilemap, pos);
					if (!canMoveUp) createCornerAboveRight(tilemap, pos);
					if (!canMoveDown) createCornerBelowRight(tilemap, pos);
				}
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

		createRoom(tilemap, { 2, 2 }, { 25, 20 }, {
			1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
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
			.resolution = {2560, 1440},
			.vsyncEnabled = false
		}
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	(void) getchar();
	
	return 0;
}
