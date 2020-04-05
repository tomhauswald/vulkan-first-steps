#pragma once

#include <bitset>

#include "vulkan_context.h"

class Mesh {
private:
	VulkanContext& m_vulkanContext;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	BufferInfo m_vbufInfo;
	BufferInfo m_ibufInfo;

	std::bitset<2> m_dirty;

private:
	inline void freeVulkanResources() {

		m_vulkanContext.flush();

		if (m_vbufInfo.buffer) {
			m_vulkanContext.destroyBuffer(m_vbufInfo);
		}

		if (m_ibufInfo.buffer) {
			m_vulkanContext.destroyBuffer(m_ibufInfo);
		}
	}

public:
	inline Mesh(VulkanContext& vulkanContext):
		m_vulkanContext{ vulkanContext },
		m_vertices{},
		m_indices{},
		m_vbufInfo{},
		m_ibufInfo{},
		m_dirty{ 0 } {
	}
	
	Mesh(Mesh&&) noexcept = default;
	
	inline ~Mesh() {
		freeVulkanResources();
	}

	inline std::vector<Vertex> const& vertices() const {
		return m_vertices;
	}

	inline std::vector<uint32_t> const& indices() const {
		return m_indices;
	}

	inline void setVertices(std::vector<Vertex> vertices) {
		m_vertices = vertices;
		m_dirty.set(0);
	}

	inline void setIndices(std::vector<uint32_t> indices) {
		m_indices = indices;
		m_dirty.set(1);
	}
	
	inline void updateVulkanBuffers() {

		// Wait for all frames in flight to be delivered before updating.
		m_vulkanContext.flush();

		freeVulkanResources();

		if (m_dirty[0]) m_vbufInfo = m_vulkanContext.createVertexBuffer(m_vertices);
		if (m_dirty[1]) m_ibufInfo = m_vulkanContext.createIndexBuffer(m_indices);
		m_dirty.reset();
	}

	// The constness of these are questionable, since we are returning
	// native handles to the vulkan buffers. However, the result of
	// this operation shall exclusively be used for read and draw
	// commands. To express this, we opt for the const.

	inline VkBuffer vulkanVertexBuffer() const {
		return m_vbufInfo.buffer;
	}

	inline VkBuffer vulkanIndexBuffer() const {
		return m_ibufInfo.buffer;
	}
};