#pragma once

#include "vulkan_context.h"

class Mesh {
private:
	VulkanContext& m_context;
	std::vector<Vertex> m_vertices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;

	void releaseBuffers() {

		if(m_vertexBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(m_context.device(), m_vertexBuffer, nullptr);
			m_vertexBuffer = VK_NULL_HANDLE;
		}
		
		if(m_vertexBufferMemory != VK_NULL_HANDLE) {
			vkFreeMemory(m_context.device(), m_vertexBufferMemory, nullptr);
			m_vertexBufferMemory = VK_NULL_HANDLE;
		}
	}

public:
	Mesh(VulkanContext& ctx) :
		m_context{ctx}, 
		m_vertices{},
		m_vertexBuffer{VK_NULL_HANDLE}, 
		m_vertexBufferMemory{VK_NULL_HANDLE} {
	}

	~Mesh() {
		releaseBuffers();
	}

	std::vector<Vertex> const& vertices() const { return m_vertices; }

	void setVertices(std::vector<Vertex> const& vertices) { 
		releaseBuffers();
		m_vertices = vertices; 
		auto [buf, mem] = m_context.createVertexBuffer(m_vertices);
		m_vertexBuffer = buf;
		m_vertexBufferMemory = mem;
	}

	void render(PushConstantData const& data) const {
		m_context.renderMesh(*this, data);
	}
	
	void appendDrawCommand(VkCommandBuffer& cmdbuf) const {
		
		crashIf(m_vertexBuffer == VK_NULL_HANDLE);

		auto const offsetZero = VkDeviceSize{};
		vkCmdBindVertexBuffers(cmdbuf, 0, 1, &m_vertexBuffer, &offsetZero);

		vkCmdDraw(cmdbuf, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
	}
};