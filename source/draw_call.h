#pragma once

#include "vulkan_context.h"
#include "view.h"

class DrawCall {
private:
	VulkanContext& m_context;
	size_t m_vertexCount;
	std::tuple<VkBuffer, VkDeviceMemory> m_vertexBuffer;
	PushConstantData m_pushConstants;

public:
	DrawCall(VulkanContext& context);
	~DrawCall();

	void setVertices(View<Vertex> const& vertices);
	void setPushConstants(PushConstantData const& data);
	
	void appendToCommandBuffer(VkCommandBuffer const& cmdbuf) const;
};