#pragma once

#include "vulkan_context.h"
#include "mesh.h"
#include "texture.h"

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

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Texture>> m_textures;

	void createWindow();

public:
	Renderer(RendererSettings const& settings);
	~Renderer();
	
	void initialize();
	bool isWindowOpen() const;
	void handleWindowEvents();

	void renderMesh(Mesh const& mesh, ShaderPushConstants const& push);

	bool tryBeginFrame();
	void endFrame();

	inline Mesh& createMesh() {
		m_meshes.push_back(std::make_unique<Mesh>(m_vulkanContext));
		return *m_meshes.back();
	}

	inline Texture& createTexture() {
		m_textures.push_back(std::make_unique<Texture>(m_vulkanContext));
		return *m_textures.back();
	}

	inline void bindTexture(Texture const& txr, uint32_t slot = 0) {
		m_vulkanContext.bindTexture(txr.vulkanTexture(), slot);
	}

	void setUniformData(ShaderUniforms const& data);
};
