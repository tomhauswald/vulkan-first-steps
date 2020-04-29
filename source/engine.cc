#include "engine.h"

void Engine::run() {
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

