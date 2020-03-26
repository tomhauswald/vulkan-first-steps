#pragma once

#include "vulkan_context.h"
#include "vulkan_draw_call.h"
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
		std::unique_ptr<VulkanDrawCall>
	> m_preparedDrawCalls;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);
	~Renderer();
	
	void initialize();
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene);
	void prepareDrawCall(std::string const& name, View<Vertex> vertices);
	void setGlobalUniformData(Uniform::Global const& uniform);
};
