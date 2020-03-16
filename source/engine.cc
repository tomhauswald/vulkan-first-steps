#include "renderer.h"


class Engine {
private:
	Renderer m_renderer;

public:
	int run() {
		while (m_renderer.isWindowOpen()) {
			m_renderer.renderScene({});
			m_renderer.handleWindowEvents();
		}
		return 0;
	}
};

int main() { 
	return Engine{}.run();
}
