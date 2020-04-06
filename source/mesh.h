#pragma once

#include <bitset>

#include "vulkan_context.h"

class Mesh {

	using index_type = uint32_t;

private:
	VulkanContext& m_vulkanContext;

	std::vector<Vertex> m_vertices;
	std::vector<index_type> m_indices;

	VulkanBufferInfo m_vbufInfo;
	VulkanBufferInfo m_ibufInfo;

private:
	inline void destroyVertexBuffer() {
		if (m_vbufInfo.buffer) {
			m_vulkanContext.flush();
			m_vulkanContext.destroyBuffer(m_vbufInfo);
		}
	}

	inline void destroyIndexBuffer() {
		if (m_ibufInfo.buffer) {
			m_vulkanContext.flush();
			m_vulkanContext.destroyBuffer(m_ibufInfo);
		}
	}

public:
	inline Mesh(VulkanContext& vulkanContext):
		m_vulkanContext{ vulkanContext },
		m_vertices{},
		m_indices{},
		m_vbufInfo{},
		m_ibufInfo{} {
	}
	
	inline ~Mesh() {
		destroyVertexBuffer();
		destroyIndexBuffer();
	}

	inline std::vector<Vertex> const& vertices() const {
		return m_vertices;
	}

	inline std::vector<index_type> const& indices() const {
		return m_indices;
	}

	inline void setVertices(std::vector<Vertex> vertices) {
		destroyVertexBuffer();
		m_vertices = std::move(vertices);
		m_vbufInfo = m_vulkanContext.createVertexBuffer(m_vertices);
	}

	inline void setIndices(std::vector<index_type> indices) {
		destroyIndexBuffer();
		m_indices = std::move(indices);
		m_ibufInfo = m_vulkanContext.createIndexBuffer(m_indices);
	}
	
	// The constness of these are questionable, since we are returning
	// native handles to the vulkan buffers. However, the result of
	// this operation shall exclusively be used for read and draw
	// commands. To express this, we opt for the const.

	inline VulkanBufferInfo const& vulkanVertexBuffer() const {
		return m_vbufInfo;
	}

	inline VulkanBufferInfo const& vulkanIndexBuffer() const {
		return m_ibufInfo;
	}
};