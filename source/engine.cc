#include "renderer.h"
#include "vertex_formats.h"
#include "vertex_uniforms.h"

class Engine {
private:
	Renderer m_renderer;

public:
	template<typename Vertex, typename Uniform>
	Engine& configureRenderer(RendererSettings const& settings) {
		m_renderer.configure<Vertex, Uniform>(settings);
		return *this;
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

	Engine{}.configureRenderer<Vertex2dColored, UniformMatrix>({
		.resolution = {1440, 900},
		.vsyncEnabled = false,
		.windowTitle = "Hello, Vulkan!",
		.vertexShaderName = "vert-2d-colored",
		.fragmentShaderName = "frag-unshaded"
	}).run();

	std::cout << "Press [ENTER] exit application..." << lf;
	(void) getchar();
	
	return 0;
}
