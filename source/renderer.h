#pragma once

#include "vulkan_context.h"
#include "mesh.h"
#include "sprite.h"
#include "camera.h"

#include <glm/gtx/transform.hpp>

struct RendererSettings {
	
	std::string windowTitle;
	glm::uvec2 resolution;
	bool vsyncEnabled;
};

struct SpriteQueue {
	Texture const* pTexture;
	size_t numSprites;
	std::vector<USpriteBatch> batches;
};

class Renderer {
private:
	RendererSettings m_settings;
	float m_aspectRatio;

	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	Mesh m_unitQuad;
	Mesh m_viewportQuad;
	Mesh m_spriteBatchMesh;

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Texture>> m_textures;
	
	std::vector<SpriteQueue> m_spriteQueues;

	void createWindow();

	void renderSpriteBatches();

public:
	inline Renderer(RendererSettings const& settings) :
		m_settings{ settings },
		m_aspectRatio{ settings.resolution.x / (float)settings.resolution.y },
		m_vulkanContext{},
		m_pWindow{},
		m_unitQuad{ m_vulkanContext },
		m_viewportQuad{ m_vulkanContext },
		m_spriteBatchMesh{ m_vulkanContext },
		m_spriteQueues{} {
	}
	
	inline ~Renderer() {
		m_vulkanContext.flush();
		if (m_pWindow) {
			glfwDestroyWindow(m_pWindow);
			m_pWindow = nullptr;
		}
		glfwTerminate();
	}
	
	void initialize();

	inline Mesh& createMesh() {
		m_meshes.push_back(std::make_unique<Mesh>(m_vulkanContext));
		return *m_meshes.back();
	}

	inline Texture& createTexture() {
		m_textures.push_back(std::make_unique<Texture>(m_vulkanContext));
		return *m_textures.back();
	}

	inline void bindTextureSlot(uint8_t slot, Texture const& txr) {
		m_vulkanContext.bindTextureSlot(slot, txr.vulkanTexture());
	}

	inline bool isWindowOpen() const {
		return m_pWindow && !glfwWindowShouldClose(m_pWindow);
	}

	inline void handleWindowEvents() {
		glfwPollEvents();
	}

	template<typename Uniforms>
	void setUniforms(Uniforms const& uniforms) {
		m_vulkanContext.setUniformData(&uniforms, sizeof(Uniforms));
	}

	template<typename PushConstants>
	void setPushConstants(PushConstants const& pushConsts) {
		m_vulkanContext.setPushConstantData(&pushConsts, sizeof(PushConstants));
	}

	inline void renderMesh(Mesh const& mesh) {
		m_vulkanContext.draw(
			mesh.vulkanVertexBuffer().buffer,
			mesh.vulkanIndexBuffer().buffer,
			static_cast<uint32_t>(mesh.vertices().size())
		);
	}

	void renderSprite(Sprite const& sprite, Camera2d const& camera);

	inline bool tryBeginFrame() {
		if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED)) {
			m_vulkanContext.flush();
			return false;
		}
		m_vulkanContext.onFrameBegin();
		return true;
	}

	inline void endFrame() {
		renderSpriteBatches();
		m_vulkanContext.onFrameEnd();
	}
};
