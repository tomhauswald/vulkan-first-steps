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

	void createRoom(Tilemap8& tilemap, glm::u64vec2 const& roomPosition, glm::u64vec2 const& roomSize) {

		static constexpr auto floorTiles = std::array{
			30, 30, 30, 30, 30, 30, 30, 30,
			30, 30, 30, 30, 30, 30, 30, 30,
			30, 30, 30, 30, 30, 30, 30, 30,
			32, 32, 32, 32, 38, 38, 38, 38,
			31, 31, 31, 31, 37, 39, 44, 45
		};

		static constexpr auto wallTopTiles = std::array{
			2, 3, 4, 75, 76
		};

		static constexpr size_t wallFrontTile = 10;
		static constexpr size_t wallLeftTile = 58;
		static constexpr size_t wallRightTile = 57;

		// floor
		for (auto dy : range(roomSize.y)) {
			auto y = roomPosition.y + dy;
			for (auto dx : range(roomSize.x)) {
				auto x = roomPosition.x + dx;
				tilemap.setTileAt({ x,y,0 }, randomPick(floorTiles));
			}
		}

		// top left corner
		tilemap.setTileAt({ roomPosition.x, roomPosition.y - 2, 1 }, 52);
		tilemap.setTileAt({ roomPosition.x, roomPosition.y - 1, 0 }, 59);

		// top right corner
		tilemap.setTileAt({ roomPosition.x + roomSize.x - 1, roomPosition.y - 2, 1 }, 53);
		tilemap.setTileAt({ roomPosition.x + roomSize.x - 1, roomPosition.y - 1, 0 }, 60);

		// bottom left corner
		tilemap.setTileAt({ roomPosition.x, roomPosition.y + roomSize.y - 1, 1 }, 66);
		tilemap.setTileAt({ roomPosition.x, roomPosition.y + roomSize.y, 0 }, 73);

		// bottom right corner
		tilemap.setTileAt({ roomPosition.x + roomSize.x - 1, roomPosition.y + roomSize.y - 1, 1 }, 67);
		tilemap.setTileAt({ roomPosition.x + roomSize.x - 1, roomPosition.y + roomSize.y, 0 }, 74);

		// horizontal walls
		for (size_t dx = 1; dx <= roomSize.x - 2; ++dx) {
			auto x = roomPosition.x + dx;
			tilemap.setTileAt({ x, roomPosition.y - 2, 1 }, randomPick(wallTopTiles));
			tilemap.setTileAt({ x, roomPosition.y - 1, 0 }, wallFrontTile);
			tilemap.setTileAt({ x, roomPosition.y + roomSize.y, 0 }, wallFrontTile);
			tilemap.setTileAt({ x, roomPosition.y + roomSize.y - 1, 1 }, randomPick(wallTopTiles));
		}

		// vertical walls
		for (auto dy : range(roomSize.y - 1)) {
			auto y = roomPosition.y + dy;
			tilemap.setTileAt({ roomPosition.x, y, 1 }, wallLeftTile);
			tilemap.setTileAt({ roomPosition.x + roomSize.x - 1, y, 1 }, wallRightTile);
		}

		auto& door = add(std::make_unique<Sprite>(*m_pDoorTexture));
		door.setTextureArea({ 0.0f, 0.0f, 0.5f, 1.0f });
		door.setSize(35.0f / 16.0f * tilemap.tileSize());
		door.setPosition({
			(roomPosition.x + roomSize.x / 2.0f) * tilemap.tileSize().x - door.size().x / 2.0f,
			roomPosition.y * tilemap.tileSize().y - door.size().y
		});
		door.setLayer(2);
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

		auto roomSideLen = 3;
		auto roomSpacing = glm::u64vec2{ 2, 4 };

		for (auto x : range(30)) {
			for (auto y : range(30)) {
				createRoom(tilemap, {
					1 + x * (roomSideLen + roomSpacing.x), 
					3 + y * (roomSideLen + roomSpacing.y) 
				}, { roomSideLen, roomSideLen });
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
