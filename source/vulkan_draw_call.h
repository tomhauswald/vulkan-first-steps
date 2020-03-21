#pragma once

#include "vulkan_context.h"
#include "view.h"

class VulkanDrawCall {
private:
	VulkanContext& m_vulkanContext;
	size_t m_vertexCount;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	std::vector<VkCommandBuffer> m_swapchainCommandBuffers;

public:
	VulkanDrawCall(VulkanContext& vulkanContext);
	~VulkanDrawCall();

	void prepare();
	
	inline VkCommandBuffer const& swapchainCommandBuffer(size_t imageIndex) const {
		return m_swapchainCommandBuffers[imageIndex];
	}
	
	template<typename Vertex>
	void setVertices(View<Vertex> const& vertices) {
		m_vertexCount = vertices.count();
		m_vertexBuffer = m_vulkanContext.createVertexBuffer(
			vertices,
			m_vertexBufferMemory
		);
	}
};