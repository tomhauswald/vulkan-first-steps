#pragma once

#include "renderer.h"
#include <chrono>

struct EngineSettings {
	RendererSettings renderer;
};

class Engine {
private:
	EngineSettings m_settings;
	Renderer m_renderer;
	std::vector<std::unique_ptr<GameObject>> m_objects;
	float m_totalTime;

public:
	inline Engine(EngineSettings const& settings) : 
		m_settings{ settings }, 
		m_renderer{ settings.renderer },
		m_objects{},
		m_totalTime{} {
		srand(time(nullptr));
		m_renderer.initialize();
	}

	template<typename Obj, typename ... Args>
	inline Obj& add(Args&& ... args) {
		m_objects.push_back(std::make_unique<Obj>(*this, std::forward<Args>(args)...));
		return dynamic_cast<Obj&>(*m_objects.back());
	}

	void run();
	
	inline Renderer& renderer() noexcept { return m_renderer; }
	
	GETTER(settings, m_settings)
	GETTER(objects, m_objects)
	GETTER(totalTime, m_totalTime)
};
