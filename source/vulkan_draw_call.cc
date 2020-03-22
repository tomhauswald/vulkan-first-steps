#include "vulkan_draw_call.h"

VulkanDrawCall::VulkanDrawCall(VulkanContext& vulkanContext) :
	m_vulkanContext{ vulkanContext },
	m_vertexCount{ 0 },
	m_vertexBuffer{},
	m_swapchainCommandBuffers{} {
}

void VulkanDrawCall::prepare() {
	m_swapchainCommandBuffers = m_vulkanContext.recordDrawCommands(
		std::get<0>(m_vertexBuffer),
		m_vertexCount
	);
}

VulkanDrawCall::~VulkanDrawCall() {
	auto [buf, mem] = m_vertexBuffer;
	vkDestroyBuffer(m_vulkanContext.device(), buf, nullptr);
	vkFreeMemory(m_vulkanContext.device(), mem, nullptr);
}
