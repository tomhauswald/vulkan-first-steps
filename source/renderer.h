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
	RendererSettings m_settings;
	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	VkVertexInputBindingDescription m_vertexBinding;
	std::vector<VkVertexInputAttributeDescription> m_vertexAttributes;
	size_t m_uniformBytes;

	std::unordered_map<
		std::string, 
		std::unique_ptr<VulkanDrawCall>
	> m_preparedDrawCalls;

	void createWindow();

public:
	~Renderer();
	
	void initialize();
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

	template<typename Vertex, typename Uniform>
	void configure(RendererSettings const& settings) {
		m_settings = settings;
		m_vertexBinding = Vertex::binding();
		m_vertexAttributes = Vertex::attributes(); 
		m_uniformBytes = sizeof(Uniform);
	}
};