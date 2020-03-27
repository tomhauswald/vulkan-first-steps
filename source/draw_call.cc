#include "draw_call.h"

DrawCall::DrawCall(VulkanContext& context) :
	m_context{ context },
	m_vertexCount{ 0 },
	m_vertexBuffer{ VK_NULL_HANDLE, VK_NULL_HANDLE },
	m_pushConstants{} {
}

void DrawCall::setVertices(View<Vertex> const& vertices) {
	m_vertexCount = vertices.count();
	m_vertexBuffer = m_context.createVertexBuffer(vertices);
}

void DrawCall::setPushConstants(PushConstantData const& data) {
	m_pushConstants = data;
}

void DrawCall::appendToCommandBuffer(VkCommandBuffer const& cmdbuf) const {

	// Update the values of the push constants for this object.
	vkCmdPushConstants(
		cmdbuf,
		m_context.pipelineLayout(),
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PushConstantData),
		&m_pushConstants
	);

	auto& [buf, _] = m_vertexBuffer;
	auto const offsetZero = VkDeviceSize{};
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &buf, &offsetZero);

	vkCmdDraw(cmdbuf, static_cast<uint32_t>(m_vertexCount), 1, 0, 0);
}

DrawCall::~DrawCall() {
	auto [buf, mem] = m_vertexBuffer;
	vkDestroyBuffer(m_context.device(), buf, nullptr);
	vkFreeMemory(m_context.device(), mem, nullptr);
}
