#include <liberupt-ecs/ecs.hh>

#include "camera_controllers.h"
#include "framerate_counter.h"
#include "player.h"
#include "room.h"

int main() {
  auto engine = Engine3d({.renderer = {.windowTitle = "Rogue",
                                       .resolution = {1280, 768},
                                       .enableVsync = true}});

  for (auto const& name : {"stonebrick_mossy", "nether_brick"}) {
    printf("Loading image '%s'...\n", name);
    engine.renderer().createTexture(name, "../assets/images/"s + name + ".png");
  }

  engine.add<FramerateCounter>();

  constexpr auto hubW = 10.0f;
  constexpr auto hubH = 5.0f;
  constexpr auto corrW = Room::doorWidth + 2 * Room::wallThickness;
  constexpr auto corrH = Room::doorHeight + 2 * Room::wallThickness;
  constexpr auto corrL = 12.0f;

  size_t roomIndex = 0;
  engine.add<Room>(
      roomIndex++, glm::vec3{0, 0, 0}, glm::vec3{hubW, hubH, hubW},
      static_cast<Dir>(Dir::North | Dir::East | Dir::South | Dir::West));

  engine.add<Room>(roomIndex++, glm::vec3{(hubW - corrW) / 2.0f, 0, hubW},
                   glm::vec3{corrW, corrH, corrL},
                   static_cast<Dir>(Dir::North | Dir::South));

  engine.add<Room>(roomIndex++, glm::vec3{hubW, 0, (hubW - corrW) / 2.0f},
                   glm::vec3{corrL, corrH, corrW},
                   static_cast<Dir>(Dir::East | Dir::West));

  engine.add<Room>(roomIndex++, glm::vec3{(hubW - corrW) / 2.0f, 0, -corrL},
                   glm::vec3{corrW, corrH, corrL},
                   static_cast<Dir>(Dir::North | Dir::South));

  engine.add<Room>(roomIndex++, glm::vec3{-corrL, 0, (hubW - corrW) / 2.0f},
                   glm::vec3{corrL, corrH, corrW},
                   static_cast<Dir>(Dir::East | Dir::West));

  engine.add<FirstPersonController>(3.0f, 6.0f, 60.0f)
      .setEye({4.5f, 2.0f, 4.5f});

  /*auto& player = engine.add<Player>();
  player.setPosition({ 3, 1, 4 });
      engine.add<ThirdPersonController>(player, 1.2f, 2.5f, 3.0f, 6.0f, 60.0f);*/

  engine.run();

  return 0;
}
