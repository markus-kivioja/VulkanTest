#pragma once

#include "Texture.h"

#include <vulkan/vulkan.h>

#include <memory>

class Material
{
public:
	Material(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, const std::string& filename);

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayuout);
private:
	std::unique_ptr<Texture> m_albedoMap;
	std::unique_ptr<Texture> m_normalMap;
	std::unique_ptr<Texture> m_displacementMap;
	std::unique_ptr<Texture> m_specularMap;

	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
};