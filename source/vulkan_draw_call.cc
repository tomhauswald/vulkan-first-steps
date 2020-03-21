#include "vulkan_draw_call.h"

VulkanDrawCall::VulkanDrawCall(VulkanContext& vulkanContext) :
	m_vulkanContext{ vulkanContext },
	m_vertexCount{ 0 },
	m_vertexBuffer{ VK_NULL_HANDLE },
	m_vertexBufferMemory{ VK_NULL_HANDLE },
	m_swapchainCommandBuffers{} {
}

void VulkanDrawCall::prepare() {
	m_swapchainCommandBuffers = m_vulkanContext.createCommandBuffers(
		m_vertexBuffer,
		m_vertexCount
	);
}

VulkanDrawCall::~VulkanDrawCall() {

	if (m_vertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(m_vulkanContext.device(), m_vertexBuffer, nullptr);
	}

	if (m_vertexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(m_vulkanContext.device(), m_vertexBufferMemory, nullptr);
	}
}