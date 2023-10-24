#pragma once

#include "Buffer.h"
#include "Renderer.h"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

class Transforms
{
public:
	Transforms(uint32_t count);

	void update(uint32_t index, glm::mat4 transform, uint32_t bufferIdx);
	void bind(VkCommandBuffer commandBuffer, uint32_t index, uint32_t bufferIdx);
private:
	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	std::unique_ptr<Buffer> m_uniformBuffer{ nullptr };

	uint32_t m_size{ 0 };
};