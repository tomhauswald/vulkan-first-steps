#include "../engine.h"
#include "../framerate_counter.h"

#include "room.h"
#include "first_person_controller.h"

#include <filesystem>

int main() {
	auto engine = Engine({
		.renderer = {
			.windowTitle = "Rogue",
			.resolution = {2560, 1440},
			.enableVsync = true,
			.enable2d = false
		}
	});

	for (auto const& name : { "stonebrick_mossy", "nether_brick" }) {
		printf("Loading image '%s'...\n", name);
		engine.renderer().createTexture(name, "../assets/images/"s + name + ".png");
	}
	
	engine.add<FramerateCounter>();	
	engine.add<FirstPersonController>();
	engine.add<Room>(0, glm::uvec3{ 0, 0, 0 }, glm::uvec3{ 7, 5, 9 });
	
	engine.run();
	return 0;
}
