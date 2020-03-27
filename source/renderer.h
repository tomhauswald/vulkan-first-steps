#pragma once

#include "vulkan_context.h"
#include "draw_call.h"
#include "view.h"

struct Scene { };

struct RendererSettings {
	
	std::string windowTitle;
	glm::uvec2 resolution;
	bool vsyncEnabled;
};

class Renderer {
private:
	RendererSettings m_settings;
	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	std::unordered_map<
		std::string, 
		std::unique_ptr<DrawCall>
	> m_drawCalls;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);
	~Renderer();
	
	void initialize();
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene);
	
	inline auto& createDrawCall(std::string const& name) {
		m_drawCalls.emplace(name, std::make_unique<DrawCall>(m_vulkanContext));
		return *m_drawCalls.at(name);
	}
	
	inline auto& getDrawCall(std::string const& name) {
		return *m_drawCalls.at(name);
	}

	void setUniformData(UniformData const& data);
};
