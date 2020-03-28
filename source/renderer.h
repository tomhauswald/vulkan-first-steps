#pragma once

#include "vulkan_context.h"
#include "mesh.h"

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

	std::unordered_map<std::string,	Mesh> m_meshes;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);
	~Renderer();
	
	void initialize();
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene);
	
	inline Mesh& createMesh(std::string const& name) {
		m_meshes.emplace(name, Mesh(m_vulkanContext));
		return m_meshes.at(name);
	}
	
	inline Mesh& getMesh(std::string const& name) {
		return m_meshes.at(name);
	}

	void setUniformData(UniformData const& data);
};
