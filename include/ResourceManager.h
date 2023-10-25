#pragma once

#include "Mesh.h"
#include "Material.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <string>

class ResourceManager
{
public:
	ResourceManager(VkPhysicalDevice physicalDevice, VkDevice device, const std::string& assetPath);

	VkDescriptorSetLayout getMaterialDescSetLayout() const { return m_materialDescSetLayout; };

private:
	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_materialDescSetLayout{ VK_NULL_HANDLE };

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Material>> m_materials;
};