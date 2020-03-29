#pragma once

#include "vulkan_context.h"

class Mesh {
private:
	VulkanContext* m_pContext;
	std::vector<Vertex> m_vertices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;

	void releaseBuffers();

public:
	Mesh();
	~Mesh();

	void setVulkanContext(VulkanContext& ctx);
	
	std::vector<Vertex> const& vertices() const;
	void setVertices(std::vector<Vertex> const& vertices);

	VkBuffer const& vertexBuffer() const;
};