#include "renderer.h"


class Engine {
private:
	Renderer m_renderer;

public:
	int run() {
		m_renderer.createWindow(1280, 720);
		while (m_renderer.isWindowOpen()) {
			m_renderer.renderScene({});
			m_renderer.handleWindowEvents();
		}
		return 0;
	}
};

int main() { return Engine{}.run(); }