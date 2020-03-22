#include "renderer.h"
#include "vertex_formats.h"

class Engine {
private:
	Renderer m_renderer;

public:
	template<typename Vertex>
	Engine& configureRenderer(RendererSettings const& settings) {
		m_renderer.configure<Vertex>(settings);
		return *this;
	}

	int run() {
		m_renderer.initialize();
		while (m_renderer.isWindowOpen()) {
			m_renderer.renderScene({});
			m_renderer.handleWindowEvents();
		}
		return 0;
	}
};

int main() { 
	Engine{}.configureRenderer<Vertex2dColored>({
		.resolution = {1440, 900},
		.vsyncEnabled = false,
		.windowTitle = "Hello, Vulkan!",
		.vertexShaderName = "vert-2d-colored",
		.fragmentShaderName = "frag-unshaded"
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	return getchar();
}
