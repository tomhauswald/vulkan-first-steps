#include "mesh.h"

Mesh::Mesh() :
	m_pContext{ nullptr },
	m_vertices{},
	m_vertexBuffer{ VK_NULL_HANDLE },
	m_vertexBufferMemory{ VK_NULL_HANDLE } {
}

Mesh::~Mesh() {
	releaseBuffers();
}

void Mesh::releaseBuffers() {
	if (m_pContext) {
		if (m_vertexBuffer) {
			vkDestroyBuffer(m_pContext->device(), m_vertexBuffer, nullptr);
			m_vertexBuffer = VK_NULL_HANDLE;
		}
		if (m_vertexBufferMemory) {
			vkFreeMemory(m_pContext->device(), m_vertexBufferMemory, nullptr);
			m_vertexBufferMemory = VK_NULL_HANDLE;
		}
	}
}

void Mesh::setVulkanContext(VulkanContext& ctx) { m_pContext = &ctx; }

std::vector<Vertex> const& Mesh::vertices() const { return m_vertices; }

void Mesh::setVertices(std::vector<Vertex> const& vertices) {
	crashIf(!m_pContext);

	// Wait for all frames in flight to be delivered before updating
	// the vertex data.
	vkDeviceWaitIdle(m_pContext->device());

	// Free previous buffer resources, if any.
	releaseBuffers();

	// Create new buffer resources.
	m_vertices = vertices;
	auto [buf, mem] = m_pContext->createVertexBuffer(m_vertices);
	m_vertexBuffer = buf;
	m_vertexBufferMemory = mem;
}

VkBuffer const& Mesh::vertexBuffer() const {
	crashIf(!m_vertexBuffer);
	return m_vertexBuffer;
}