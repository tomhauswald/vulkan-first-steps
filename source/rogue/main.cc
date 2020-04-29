#include "../engine.h"
#include "room.h"

#include <glm/gtx/rotate_vector.hpp>

class FirstPersonCtrl : public GameObject {
private:
	Camera3d& m_cam;
	glm::vec3 m_eye;
	float m_yaw;

public:
	FirstPersonCtrl(Engine& e) : 
		GameObject(e),
       		m_cam(e.renderer().camera3d()),
       		m_eye(0.0f, 1.5f, 0.0f),
       		m_yaw(0.0f) {
	}
	
	void update(float dt) override {
		m_cam.setPosition(m_eye);
		m_cam.lookAt(m_eye + glm::rotateY(glm::vec3{ 0, 0, 1 }, glm::radians(m_yaw)));
		m_yaw += dt * 45.0f;
		GameObject::update(dt);
	}
};

int main() {
	auto engine = Engine({
		.renderer = {
			.windowTitle = "Rogue",
			.resolution = {1600, 900},
			.enableVsync = false,
			.enable2d = false
		}
	});

	engine.renderer().createTexture("wall").updatePixelsWithImage("../assets/images/wall.png");
	
	engine.add<FirstPersonCtrl>();
	engine.add<Room>(0);
	
	engine.run();
	return 0;
}
