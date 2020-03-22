#pragma once

#include "vulkan_context.h"
#include "vulkan_draw_call.h"
#include "view.h"

struct Scene { };

struct RendererSettings {
	glm::uvec2 resolution;
	bool vsyncEnabled;
	std::string windowTitle;
	std::string vertexShaderName;
	std::string fragmentShaderName;
};

class Renderer {
private:
	RendererSettings const& m_settings;
	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	std::unordered_map<
		std::string, 
		std::unique_ptr<VulkanDrawCall>
	> m_preparedDrawCalls;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);

	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene);

	template<typename Vertex>
	void prepareDrawCall(std::string const& name, View<Vertex> vertices) {
		auto call = std::make_unique<VulkanDrawCall>(m_vulkanContext);
		call->setVertices(vertices);
		call->prepare();
		m_preparedDrawCalls[name] = std::move(call);
	}

	~Renderer();
};