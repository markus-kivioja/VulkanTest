#pragma once

#include "Buffer.h"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

class Transforms
{
public:
	Transforms(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t count);

	VkDescriptorSetLayout getDescLayout() const { return m_descSetLayout; };

	void update(uint32_t index, glm::mat4 transform, uint32_t bufferIdx);
	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, uint32_t index);
private:
	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_descSetLayout{ VK_NULL_HANDLE };
	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };

	std::unique_ptr<Buffer> m_uniformBuffer{ nullptr };

	uint32_t m_size{ 0 };
};