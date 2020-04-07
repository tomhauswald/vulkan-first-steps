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

	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

	Mesh m_fsqMesh;
	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Texture>> m_textures;

	ShaderUniforms m_uniforms;

	void createWindow();

public:
	inline Renderer(RendererSettings const& settings) :
		m_settings{ settings },
		m_vulkanContext{},
		m_pWindow{},
		m_fsqMesh{ m_vulkanContext },
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

	inline void bindTexture(Texture const& txr, uint32_t slot = 0) {
		m_vulkanContext.bindTexture(txr.vulkanTexture(), slot);
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
		});
		bindTexture(*sprite.texture());
		renderMesh(m_fsqMesh, push);
	}

	inline bool tryBeginFrame() {
		if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED)) {
			m_vulkanContext.flush();
			return false;
		}
		m_vulkanContext.setUniforms(m_uniforms);
		m_vulkanContext.onFrameBegin();
		return true;
	}

	inline void endFrame() {
		m_vulkanContext.onFrameEnd();
	}

	SETTER(setCameraTransform, m_uniforms.cameraTransform)
};
