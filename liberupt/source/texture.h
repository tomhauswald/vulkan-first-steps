#pragma once

#include "vulkan_context.h"

class Texture {
  using pixel_type = uint32_t;

 private:
  VulkanContext& m_vulkanContext;
  std::vector<pixel_type> m_pixels;
  VulkanTextureInfo m_txrInfo;

 private:
  inline void destroyTexture() {
    if (m_txrInfo.image) {
      m_vulkanContext.flush();
      m_vulkanContext.destroyTexture(m_txrInfo);
    }
  }

 public:
  inline Texture(VulkanContext& vulkanContext)
      : m_vulkanContext{vulkanContext}, m_pixels{}, m_txrInfo{} {}

  inline ~Texture() { destroyTexture(); }

  GETTER(width, m_txrInfo.width)
  GETTER(height, m_txrInfo.height)
  GETTER(pixels, m_pixels)

  inline void updatePixelsWithImage(std::string const& path) {
    updatePixelsWithImage(png::image<png::rgba_pixel>(path.c_str()));
  }

  inline void updatePixelsWithImage(png::image<png::rgba_pixel> const& image) {
    auto w = image.get_width();
    auto h = image.get_height();
    auto pixels = std::vector<uint32_t>(static_cast<size_t>(w) * h);

    for (auto y : range(h)) {
      for (auto x : range(w)) {
        auto pixel = image.get_pixel(x, y);
        pixels[x + y * w] =
            pixel.alpha << 24 | pixel.blue << 16 | pixel.green << 8 | pixel.red;
      }
    }

    updatePixels(w, h, std::move(pixels));
  }

  inline void updatePixels(uint32_t width, uint32_t height,
                           std::vector<pixel_type> pixels) {
    destroyTexture();
    m_pixels = std::move(pixels);
    m_txrInfo = m_vulkanContext.createTexture(width, height, m_pixels.data());
  }

  // The constness of these are questionable, since we are returning
  // native handles to the vulkan buffers. However, the result of
  // this operation shall exclusively be used for read and draw
  // commands. To express this, we opt for the const.

  GETTER(vulkanTexture, m_txrInfo)
};