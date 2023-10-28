#include "ResourceManager.h"

#include <filesystem>

ResourceManager::ResourceManager(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx, const std::string& assetPath)
{
    const uint32_t materialCount = MATERIAL_FILES.size();

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

    VkCommandPoolCreateInfo commanPoolCreateInfo{};
    commanPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commanPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commanPoolCreateInfo.queueFamilyIndex = queueFamilyIdx;
    VkCommandPool copyCommandPool{ VK_NULL_HANDLE };
    result = vkCreateCommandPool(device, &commanPoolCreateInfo, nullptr, &copyCommandPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create copy command pool" << std::endl;
        std::terminate();
    }

    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = copyCommandPool;
    commandBufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &copyCommandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);

    for (const auto& materialFile : MATERIAL_FILES)
    {
        m_materials.push_back(std::make_unique<Material>(physicalDevice, device, copyCommandBuffer, m_materialDescSetLayout, materialFile));
    }

    ImGui_ImplVulkan_CreateFontsTexture(copyCommandBuffer);

    vkEndCommandBuffer(copyCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    vkFreeCommandBuffers(device, copyCommandPool, 1, &copyCommandBuffer);
    vkDestroyCommandPool(device, copyCommandPool, nullptr);


}