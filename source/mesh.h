#pragma once

#include <bitset>

#include "vulkan_context.h"

class Mesh {
private:
	VulkanContext& m_vulkanContext;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	std::tuple<VkBuffer, VkDeviceMemory> m_vertexBuffer;
	std::tuple<VkBuffer, VkDeviceMemory> m_indexBuffer;

	std::bitset<2> m_dirty;

private:
	inline void freeVulkanResources() {

		m_vulkanContext.flush();

		if (m_vertices.size()) {
			m_vulkanContext.destroyBuffer(m_vertexBuffer);
		}

		if (m_indices.size()) {
			m_vulkanContext.destroyBuffer(m_indexBuffer);
		}
	}

public:
	inline Mesh(VulkanContext& vulkanContext):
		m_vulkanContext{ vulkanContext },
		m_vertices{},
		m_indices{},
		m_vertexBuffer{ VK_NULL_HANDLE, VK_NULL_HANDLE },
		m_indexBuffer{ VK_NULL_HANDLE, VK_NULL_HANDLE },
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

		if (m_dirty[0]) m_vertexBuffer = m_vulkanContext.createVertexBuffer(m_vertices);
		if (m_dirty[1]) m_indexBuffer = m_vulkanContext.createIndexBuffer(m_indices);
		m_dirty.reset();
	}

	// The constness of this is questionable, since we are returning
	// native handles to the vulkan buffers. However, the result of
	// this operation shall exclusively be used for read and draw
	// commands. To express this, we opt for the const.
	inline std::pair<VkBuffer, VkBuffer> vulkanBufferHandles() const {
		return { 
			std::get<VkBuffer>(m_vertexBuffer), 
			std::get<VkBuffer>(m_indexBuffer) 
		};
	}
};