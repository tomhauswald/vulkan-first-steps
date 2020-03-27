#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>

#include "common.h"

template<typename Vertex>
auto defaultDescribeVertexInputBinding(uint32_t binding) {
	return VkVertexInputBindingDescription{
		.binding = binding,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
}

template<typename Vertex>
auto describeVertexInputBinding(uint32_t binding) {
	return defaultDescribeVertexInputBinding<Vertex>(binding);
}

template<typename Attribute>
constexpr auto vertexAttributeFormat() { return VK_FORMAT_UNDEFINED; }

template<> constexpr auto vertexAttributeFormat<float>() { return VK_FORMAT_R32_SFLOAT; }
template<> constexpr auto vertexAttributeFormat<glm::vec2>() { return VK_FORMAT_R32G32_SFLOAT; }
template<> constexpr auto vertexAttributeFormat<glm::vec3>() { return VK_FORMAT_R32G32B32_SFLOAT; }
template<> constexpr auto vertexAttributeFormat<glm::vec4>() { return VK_FORMAT_R32G32B32A32_SFLOAT; }

template<typename Attribute>
auto describeVertexInputAttribute(uint32_t location, uint32_t offset, uint32_t binding = 0) {
	return VkVertexInputAttributeDescription{
		.location = location,
		.binding = binding,
		.format = vertexAttributeFormat<Attribute>(),
		.offset = offset
	};
}

template<typename ... Attributes>
auto describeVertexInputAttributes(uint32_t binding) {
	
	uint32_t location = 0;
	uint32_t offset = 0;
	
	return std::vector{
		describeVertexInputAttribute<Attributes>(
			location++, 
			([&]() {
				auto os = offset;
				offset += sizeof(Attributes);
				return os;
			}) (), 
			binding
		)...
	};
}

struct Vertex {
	
	glm::vec3 position;
	glm::vec3 color;

	static inline auto binding() {
		return describeVertexInputBinding<Vertex>(0);
	}

	static inline auto attributes() {
		return describeVertexInputAttributes<
			glm::vec3, 
			glm::vec3
		>(0); 
	}
};
