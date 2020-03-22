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
	
	Engine{}.configureRenderer({
		.resolution = {1440, 900},
		.vsyncEnabled = false,
		.windowTitle = "Hello, Vulkan!",
		.vertexShaderName = "vert-2d-colored",
		.fragmentShaderName = "frag-unshaded"
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	return getchar();
}
