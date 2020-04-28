#pragma once

#include "vulkan_context.h"
#include "mesh.h"
#include "camera.h"
#include "sprite.h"

#include <map>
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

	Mesh m_unitQuad;
	Mesh m_viewportQuad;
	Mesh m_spriteBatchMesh;

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Texture>> m_textures;
	
	Camera2d m_camera2d;
	Camera3d m_camera3d;

	std::array<
		std::map<Texture const*, std::pair<size_t, std::vector<USpriteBatch>>>,
		Sprite::numLayers
	> m_layerSpriteBatches;

	void createWindow();

	void renderSpriteBatches();

public:
	inline Renderer(RendererSettings const& settings) :
		m_settings{ settings },
		m_aspectRatio{ settings.resolution.x / (float)settings.resolution.y },
		m_pWindow{},
		m_vulkanContext{},
		m_unitQuad{ m_vulkanContext },
		m_viewportQuad{ m_vulkanContext },
		m_spriteBatchMesh{ m_vulkanContext },
		m_camera2d({ m_settings.resolution.x, m_settings.resolution.y }),
		m_camera3d{ m_aspectRatio, 45.0f } {
	}
	
	inline ~Renderer() {
		m_vulkanContext.flush();
		if (m_pWindow) {
			glfwDestroyWindow(m_pWindow);
			m_pWindow = nullptr;
		}
		glfwTerminate();
	}
	
	void initialize(bool mode2d);

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

	inline void renderSprite(Sprite const& sprite) {
		if (m_camera2d.isWorldRectVisible(sprite.position(), sprite.size())) {
			
			auto& [numSprites, batches] = m_layerSpriteBatches[sprite.layer()][&sprite.texture()];
			batches.resize(std::ceil((numSprites + 1) / (float)USpriteBatch::size));

			auto& batch = batches.back();
			auto k = numSprites % USpriteBatch::size;

			batch.bounds[k] = m_camera2d.worldToNdcRect(sprite.position(), sprite.size());
			batch.textureAreas[k] = sprite.textureArea();
			batch.colors[k] = sprite.color();
			batch.trigonometry[k / 2][2 * (k % 2) + 0] = glm::sin(glm::radians(sprite.rotation()));
			batch.trigonometry[k / 2][2 * (k % 2) + 1] = glm::cos(glm::radians(sprite.rotation()));

			++numSprites;
		}
	}

	inline bool tryBeginFrame() {
		if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED)) {
			m_vulkanContext.flush();
			return false;
		}
		m_vulkanContext.onFrameBegin();
		setUniforms(UCameraTransform{m_camera3d.transform()});
		return true;
	}

	inline void endFrame() {
		renderSpriteBatches();
		m_vulkanContext.onFrameEnd();
	}

	GETTER(camera2d, m_camera2d)
	GETTER(camera3d, m_camera3d)

	SETTER(setCamera2d, m_camera2d)
	SETTER(setCamera3d, m_camera3d)
};
