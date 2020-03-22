#pragma once

#include "vulkan_context.h"
#include "vulkan_draw_call.h"
#include "view.h"

struct Scene { };

struct RendererSettings {
	glm::uvec2 resolution;
	bool vsyncEnabled;
	std::string windowTitle;
};

class Renderer {
private:
	RendererSettings const& m_settings;

	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	std::unordered_map<std::string, VulkanDrawCall> m_preparedDrawCalls;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);

	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene);

	template<typename Vertex>
	void prepareDrawCall(std::string const& name, View<Vertex> vertices) {
		m_preparedDrawCalls.try_emplace(name, VulkanDrawCall(m_vulkanContext));
		m_preparedDrawCalls.at(name).setVertices(vertices);
		m_preparedDrawCalls.at(name).prepare();
	}

	~Renderer();
};