#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <string>

class Mesh;
class Material;

class ResourceManager
{
public:
	ResourceManager(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx, const std::string& assetPath);

	VkDescriptorSetLayout getMaterialDescSetLayout() const { return m_materialDescSetLayout; };

private:
	inline static const std::vector<std::string> MATERIAL_FILES = {
		"crate.jpg",
		"mickey.png"
	};
	inline static const std::string MESH_FILES = {
		"mickey.obj"
	};

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_materialDescSetLayout{ VK_NULL_HANDLE };

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Material>> m_materials;
};