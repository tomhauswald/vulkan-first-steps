#pragma once

#include <glm/matrix.hpp>
#include <vulkan/vulkan.h>

#include "common.h"

struct VulkanLimits {
	static constexpr size_t maxUniformBufferRange = 16384;
	static constexpr size_t minUniformBufferOffsetAlignment = 256;
	static constexpr size_t maxPushConstantsSize = 128;
	static constexpr size_t maxBoundDescriptorSets = 4;
	static constexpr size_t maxDescriptorSetUniformBuffersDynamic = 8;
};

template<typename VPositionColorTexcoord>
auto defaultDescribeVertexInputBinding(uint32_t binding) {
	return VkVertexInputBindingDescription{
		.binding = binding,
		.stride = sizeof(VPositionColorTexcoord),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
}

template<typename VPositionColorTexcoord>
auto describeVertexInputBinding(uint32_t binding) {
	return defaultDescribeVertexInputBinding<VPositionColorTexcoord>(binding);
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

struct VPositionColorTexcoord {
	glm::vec3 position;
	glm::vec3 m_color;
	glm::vec2 texcoord;

	static inline auto binding() {
		return describeVertexInputBinding<VPositionColorTexcoord>(0);
	}

	static inline auto attributes() {
		return describeVertexInputAttributes<
			glm::vec3, 
			glm::vec3,
			glm::vec2
		>(0); 
	}

};

struct UCameraTransform {
	glm::mat4 cameraTransform;
};

struct USpriteBatch {
	static constexpr auto size = 292;
	
	std::array<glm::vec4, size> bounds;
	std::array<glm::vec4, size> textureAreas;
	std::array<glm::vec4, size> colors;
	std::array<glm::vec4, size / 2> trigonometry;

	inline USpriteBatch() : bounds{}, textureAreas{}, colors{}, trigonometry{} {
	}
};

struct PCInstanceTransform {
	glm::mat4 modelMatrix;
	static constexpr auto a = sizeof(USpriteBatch);
};
