#include "renderer.h"
#include <glm/gtx/euler_angles.hpp>
#include <string>
#include <vulkan/vulkan_core.h>

inline void renderMesh(VulkanContext &ctx, Mesh const &mesh) {
  ctx.draw(mesh.vulkanVertexBuffer().buffer, mesh.vulkanIndexBuffer().buffer,
           static_cast<uint32_t>(mesh.vertices().size()));
}

void Renderer3d::renderModel(Model const &model) {
  bindTextureSlot(0, model.texture());
  setPushConstants(PCInstanceTransform{
      glm::translate(model.position()) * glm::scale(model.scale()) *
      glm::eulerAngleYXZ(model.euler().y, model.euler().x, model.euler().z)});
  renderMesh(m_vulkanContext, model.mesh());
}

void Renderer::materialize(VulkanPipelineSettings const &pipelineSettings) {

  createWindow();

  m_vulkanContext.createInstance();
  m_vulkanContext.accomodateWindow(m_pWindow);
  m_vulkanContext.selectPhysicalDevice();
  m_vulkanContext.createDevice();
  m_vulkanContext.createSwapchain(m_settings.enableVsync);
  m_vulkanContext.createDepthBuffer();
  m_vulkanContext.createPipeline(pipelineSettings);

  glfwShowWindow(m_pWindow);
}

void Renderer2d::renderSpriteBatches() {
  for (auto const &layer : m_layerSpriteBatches) {
    for (auto const &[pTexture, mapEntry] : layer) {
      auto const &[numSprites, batches] = mapEntry;

      // Clear sprite batch data behind last entry in last batch.
      auto emptyBytes =
          (USpriteBatch::size - 1) - (numSprites % USpriteBatch::size);
      auto where =
          const_cast<char *>(reinterpret_cast<char const *>(&batches.back()) +
                             sizeof(USpriteBatch) - emptyBytes);
      memset(where, 0, emptyBytes);

      bindTextureSlot(0, *pTexture);
      for (auto const &batch : batches) {
        setUniforms(batch);
        renderMesh(m_vulkanContext, m_spriteBatchMesh);
      }
    }
  }

  // Reset before next frame.
  m_layerSpriteBatches = {};
}

void Renderer::createWindow() {

  // Initialize GLFW.
  crashIf(!glfwInit());

  // Create GLFW window.
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  m_pWindow =
      glfwCreateWindow(static_cast<int>(m_settings.resolution.x),
                       static_cast<int>(m_settings.resolution.y),
                       m_settings.windowTitle.c_str(), nullptr, nullptr);

  // Window creation must succeed.
  crashIf(!m_pWindow);

  m_keyboard.listen(m_pWindow);
  m_mouse.listen(m_pWindow);
}
