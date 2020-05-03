#include "../engine.h"
#include "../framerate_counter.h"

#include "room.h"
#include "player.h"
#include "camera_controllers.h"

#include <filesystem>

int main() {
	auto engine = Engine({
		.renderer = {
			.windowTitle = "Rogue",
			.resolution = {1920, 1030},
			.enableVsync = true,
			.enable2d = false
		}
	});

	for (auto const& name : { "stonebrick_mossy", "nether_brick" }) {
		printf("Loading image '%s'...\n", name);
		engine.renderer().createTexture(name, "../assets/images/"s + name + ".png");
	}
	
	engine.add<FramerateCounter>();	
	engine.add<Room>(0, glm::uvec3{ 0, 0, 0 }, glm::uvec3{ 7, 5, 9 });
    
    auto& player = engine.add<Player>();
    player.setPosition({ 3, 1, 4 });
    
	engine.add<ThirdPersonController>(player, 1.2f, 2.5f, 3.0f, 6.0f, 60.0f);
	
	engine.run();
	return 0;
}
