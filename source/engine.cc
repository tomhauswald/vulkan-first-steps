#include "renderer.h"

struct EngineSettings {
	RendererSettings renderer;
};

class Engine {
private:
	[[maybe_unused]]
	EngineSettings const& m_settings;
	
	Renderer m_renderer;

public:
	Engine(EngineSettings const& settings) : 
		m_settings{ settings }, 
		m_renderer{ settings.renderer } {
	}

	void run() {
		m_renderer.initialize();
		while (m_renderer.isWindowOpen()) {
			m_renderer.renderScene({});
			m_renderer.handleWindowEvents();
		}
	}
};

int main() { 
	Engine({
		.renderer = {
			.windowTitle = "Vulkan Renderer",
			.resolution = {1440, 900},
			.vsyncEnabled = false
		}
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	(void) getchar();
	
	return 0;
}
