#include "renderer.h"


class Engine {
private:
	std::unique_ptr<Renderer> m_pRenderer;

public:
	Engine& configureRenderer(RendererSettings const& settings) {
		m_pRenderer = std::make_unique<Renderer>(settings);
		return *this;
	}

	int run() {
		while (m_pRenderer->isWindowOpen()) {
			m_pRenderer->renderScene({});
			m_pRenderer->handleWindowEvents();
		}
		return 0;
	}
};

int main() { 
	return Engine{}.configureRenderer({ 
		.resolution = {1440, 900},
		.vsyncEnabled = true,
		.windowTitle = "Hello, Vulkan!"
	}).run();
}
