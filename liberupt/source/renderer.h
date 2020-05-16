#pragma once

#include <vulkan/vulkan_core.h>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#endif

#include <map>
#include <unordered_map>

#include "camera.h"
#include "keyboard.h"
#include "mesh.h"
#include "model.h"
#include "mouse.h"
#include "shader_interface.h"
#include "sprite.h"

struct RendererSettings {
  std::string windowTitle;
  glm::uvec2 resolution;
  bool enableVsync;
};

class Renderer {
 private:
  Keyboard m_keyboard;
  Mouse m_mouse;

  std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;

  void createWindow();

 protected:
  RendererSettings m_settings;
  float m_aspectRatio;

  GLFWwindow* m_pWindow;
  VulkanContext m_vulkanContext;

  virtual void onFrameBegin() = 0;
  virtual void onFrameEnd() = 0;

  void materialize(VulkanPipelineSettings const& pipelineSettings);

 public:
  inline Renderer(RendererSettings settings)
      : m_settings(std::move(settings)),
        m_aspectRatio(settings.resolution.x / (float)settings.resolution.y),
        m_pWindow(nullptr),
        m_vulkanContext() {}

  inline ~Renderer() {
    m_vulkanContext.flush();
    if (m_pWindow) {
      glfwDestroyWindow(m_pWindow);
      m_pWindow = nullptr;
    }
    glfwTerminate();
  }

  inline Texture& createTexture(std::string const& name) {
    m_textures[name] = std::make_unique<Texture>(m_vulkanContext);
    return *m_textures.at(name);
  }

  inline Texture& createTexture(std::string const& name,
                                std::string const& imagePath) {
    auto& texture = createTexture(name);
    texture.updatePixelsWithImage(imagePath);
    return texture;
  }

  inline Texture& texture(std::string const& name) {
    return *m_textures.at(name);
  }

  inline void bindTextureSlot(uint8_t slot, Texture const& txr) {
    m_vulkanContext.bindTextureSlot(slot, txr.vulkanTexture());
  }

  inline bool isWindowOpen() const {
    return m_pWindow && !glfwWindowShouldClose(m_pWindow);
  }

  inline void handleWindowEvents() {
    m_keyboard.resetKeyStates();
    m_mouse.resetButtonStates();
    glfwPollEvents();
    m_mouse.updateCursorPosition();
  }

  template <typename Uniforms>
  void setUniforms(Uniforms const& uniforms) {
    m_vulkanContext.setUniformData(&uniforms, sizeof(Uniforms));
  }

  template <typename PushConstants>
  void setPushConstants(PushConstants const& pushConsts) {
    m_vulkanContext.setPushConstantData(&pushConsts, sizeof(PushConstants));
  }

  inline bool tryBeginFrame() {
    // Skip frame if window is minimized.
    if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED)) {
      m_vulkanContext.flush();
      return false;
    }

    m_vulkanContext.onFrameBegin();
    onFrameBegin();
    return true;
  }

  inline void endFrame() {
    onFrameEnd();
    m_vulkanContext.onFrameEnd();
  }

  inline Mouse& mouse() noexcept { return m_mouse; }

  GETTER(keyboard, m_keyboard)
  GETTER(settings, m_settings)
};

class Renderer2d : public Renderer {
 private:
  Camera2d m_camera2d;
  Mesh m_spriteBatchMesh;

  std::array<
      std::map<Texture const*, std::pair<size_t, std::vector<USpriteBatch>>>,
      Sprite::numLayers>
      m_layerSpriteBatches;

  void renderSpriteBatches();

  void onFrameBegin() override {}
  void onFrameEnd() override { renderSpriteBatches(); }

 public:
  inline Renderer2d(RendererSettings settings)
      : Renderer(std::move(settings)),
        m_camera2d({m_settings.resolution.x, m_settings.resolution.y}),
        m_spriteBatchMesh(m_vulkanContext) {
    const VPositionColorTexcoord unitQuadVertices[6] = {
        {{0, 0, 0}, {1, 1, 1}, {0, 0}}, {{1, 0, 0}, {1, 1, 1}, {1, 0}},
        {{1, 1, 0}, {1, 1, 1}, {1, 1}}, {{1, 1, 0}, {1, 1, 1}, {1, 1}},
        {{0, 1, 0}, {1, 1, 1}, {0, 1}}, {{0, 0, 0}, {1, 1, 1}, {0, 0}}};

    auto spriteBatchVertices =
        std::vector<VPositionColorTexcoord>(6 * USpriteBatch::size);

    for (auto i : range(spriteBatchVertices.size())) {
      spriteBatchVertices[i] = unitQuadVertices[i % 6];
    }

    m_spriteBatchMesh.setVertices(std::move(spriteBatchVertices));
  }

  inline void renderSprite(Sprite const& sprite) {
    if (m_camera2d.isWorldRectVisible(sprite.position(), sprite.size())) {
      auto& [numSprites, batches] =
          m_layerSpriteBatches[sprite.layer()][&sprite.texture()];
      batches.resize(std::ceil((numSprites + 1) / (float)USpriteBatch::size));

      auto& batch = batches.back();
      auto k = numSprites % USpriteBatch::size;

      batch.bounds[k] =
          m_camera2d.worldToNdcRect(sprite.position(), sprite.size());
      batch.textureAreas[k] = sprite.textureArea();
      batch.colors[k] = sprite.color();
      batch.trigonometry[k / 2][2 * (k % 2) + 0] =
          glm::sin(glm::radians(sprite.rotation()));
      batch.trigonometry[k / 2][2 * (k % 2) + 1] =
          glm::cos(glm::radians(sprite.rotation()));

      ++numSprites;
    }
  }

  inline void materialize() {
    VulkanPipelineSettings ps;
    ps.vertexInputAttribs = VPositionColorTexcoord::attributes();
    ps.vertexInputBinding = VPositionColorTexcoord::binding();
    ps.vertexShaderPath = "../assets/shaders/spirv/vert-sprite.spv";
    ps.fragmentShaderPath = "../assets/shaders/spirv/frag-textured.spv";
    ps.textureFilterMode = VK_FILTER_NEAREST;
    ps.enableDepthTest = false;

    Renderer::materialize(ps);
  }

  inline Camera2d& camera2d() noexcept { return m_camera2d; }
};

class Renderer3d : public Renderer {
 private:
  Camera3d m_camera3d;

  std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;

  void onFrameBegin() override {
    setUniforms(UCameraTransform{m_camera3d.transform()});
  }
  void onFrameEnd() override {}

 public:
  inline Renderer3d(RendererSettings settings)
      : Renderer(std::move(settings)), m_camera3d(m_aspectRatio, 45.0f) {}

  inline void materialize() {
    VulkanPipelineSettings ps;
    ps.vertexInputAttribs = VPositionColorTexcoord::attributes();
    ps.vertexInputBinding = VPositionColorTexcoord::binding();
    ps.vertexShaderPath = "../assets/shaders/spirv/vert-textured.spv";
    ps.fragmentShaderPath = "../assets/shaders/spirv/frag-textured.spv";
    ps.textureFilterMode = VK_FILTER_LINEAR;
    ps.enableDepthTest = true;

    Renderer::materialize(ps);
  }

  void renderModel(Model const& model);

  inline Camera3d& camera3d() noexcept { return m_camera3d; }

  inline Mesh& createMesh(std::string const& name) {
    m_meshes[name] = std::make_unique<Mesh>(m_vulkanContext);
    return *m_meshes.at(name);
  }

  inline Mesh& mesh(std::string const& name) { return *m_meshes.at(name); }
};
