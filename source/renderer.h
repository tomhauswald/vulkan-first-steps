#pragma once

#include "vulkan_context.h"
#include "mesh.h"

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

	std::vector<Mesh> m_meshes;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);
	~Renderer();
	
	void initialize();
	bool isWindowOpen() const;
	void handleWindowEvents();

	void renderMesh(Mesh const& mesh, PushConstantData const& data);

	bool tryBeginFrame();
	void endFrame();

	inline Mesh& createMesh() {
		m_meshes.push_back({});
		m_meshes.back().setVulkanContext(m_vulkanContext);
		return m_meshes.back();
	}
	
	void setUniformData(UniformData const& data);
};
