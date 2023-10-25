#include "ResourceManager.h"

ResourceManager::ResourceManager(VkPhysicalDevice physicalDevice, VkDevice device, const std::string& assetPath)
{
    VkDescriptorPoolSize materialDescPoolSize{};
    materialDescPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialDescPoolSize.descriptorCount = materialCount;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    std::array<VkDescriptorPoolSize, 1> poolSizes{ materialDescPoolSize };
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.maxSets = materialCount;

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor pool" << std::endl;
        std::terminate();
    }

    VkDescriptorSetLayoutBinding materialLayoutBinding{};
    materialLayoutBinding.binding = 1;
    materialLayoutBinding.descriptorCount = 1;
    materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialLayoutBinding.pImmutableSamplers = nullptr;
    materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo materialDescSetLayoutInfo{};
    materialDescSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayoutBinding, 1> modelBindings = { materialLayoutBinding };
    materialDescSetLayoutInfo.bindingCount = static_cast<uint32_t>(modelBindings.size());
    materialDescSetLayoutInfo.pBindings = modelBindings.data();

    result = vkCreateDescriptorSetLayout(device, &materialDescSetLayoutInfo, nullptr, &m_materialDescSetLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create model descriptor set layout!" << std::endl;
        std::terminate();
    }
}