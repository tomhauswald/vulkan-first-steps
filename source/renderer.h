#pragma once

#include "vulkan_context.h"
#include "mesh.h"
#include "sprite.h"

#include <glm/gtx/transform.hpp>

struct RendererSettings {
	
	std::string windowTitle;
	glm::uvec2 resolution;
	bool vsyncEnabled;
};

class Renderer {
private:
	RendererSettings m_settings;
	float m_aspectRatio;

	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	Mesh m_unitQuad, m_viewportQuad;
	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Texture>> m_textures;

	ShaderUniforms m_uniforms;

	void createWindow();

public:
	inline Renderer(RendererSettings const& settings) :
		m_settings{ settings },
		m_aspectRatio{ settings.resolution.x / (float)settings.resolution.y },
		m_vulkanContext{},
		m_pWindow{},
		m_unitQuad{ m_vulkanContext },
		m_viewportQuad{ m_vulkanContext },
		m_uniforms{} {
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

	inline void renderMesh(Mesh const& mesh, ShaderPushConstants const& push) {
		m_vulkanContext.setPushConstants(push);
		m_vulkanContext.draw(
			mesh.vulkanVertexBuffer().buffer,
			mesh.vulkanIndexBuffer().buffer,
			static_cast<uint32_t>(mesh.vertices().size())
		);
	}

	inline void renderSprite(Sprite const& sprite) {
		
		auto push = ShaderPushConstants{};
		push.modelMatrix = glm::translate(glm::vec3{
			sprite.bounds().x,
			sprite.bounds().y,
			0.0f
		}) * glm::scale(glm::vec3{
			sprite.bounds().w,
			sprite.bounds().h,
			1.0f
		});
		
		bindTextureSlot(0, sprite.texture());
		renderMesh(m_unitQuad, push);
	}

	inline bool tryBeginFrame() {
		if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED)) {
			m_vulkanContext.flush();
			return false;
		}
		m_vulkanContext.onFrameBegin();
		return true;
	}

	inline void endFrame() {
		m_vulkanContext.onFrameEnd();
	}

	inline void setCameraTransform(glm::mat4 cameraTransform) {
		m_uniforms.cameraTransform = std::move(cameraTransform);
		m_vulkanContext.setUniforms(m_uniforms);
	}
};
