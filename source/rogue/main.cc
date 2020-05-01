#include "../engine.h"
#include "../framerate_counter.h"

#include "room.h"

#include <glm/gtx/rotate_vector.hpp>

class FirstPersonCtrl : public GameObject {
private:
	Camera3d& m_cam;
	glm::vec3 m_eye;
	glm::vec3 m_forward;
	bool m_centerCursor;

public:
	FirstPersonCtrl(Engine& e) : 
		GameObject(e),
		m_cam(e.renderer().camera3d()),
       		m_eye(0.0f, 1.5f, 0.0f),
       		m_forward(0, 0, 1),
       		m_centerCursor(true) {
	}
	
	void update(float dt) override {
		
		if(m_engine.renderer().keyboard().pressed(GLFW_KEY_SPACE))
			m_centerCursor = !m_centerCursor;

		m_engine.renderer().mouse().setCursorMode(
			m_centerCursor 
			? CursorMode::Centered 
			: CursorMode::Normal
		);
		
		constexpr auto radPerSec = glm::radians(90.0f);
		auto rotate = 0.0f;
		if(m_engine.renderer().keyboard().down(GLFW_KEY_A)) rotate -= radPerSec;
		if(m_engine.renderer().keyboard().down(GLFW_KEY_D)) rotate += radPerSec;
		m_forward = glm::rotateY(m_forward, rotate * dt);
	       	
		constexpr auto unitsPerSec = 5.0f;
		auto move = 0.0f;
		if(m_engine.renderer().keyboard().down(GLFW_KEY_W)) move += unitsPerSec;
		if(m_engine.renderer().keyboard().down(GLFW_KEY_S)) move -= unitsPerSec;
		m_eye += m_forward * move * dt;

		m_cam.setPosition(m_eye);
		m_cam.lookAt(m_eye + m_forward);
		
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
	
	engine.add<FramerateCounter>();	
	engine.add<FirstPersonCtrl>();
	engine.add<Room>(0);
	
	engine.run();
	return 0;
}
