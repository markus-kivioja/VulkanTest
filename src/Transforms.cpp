#include "Transforms.h"

#include <iostream>

Transforms::Transforms(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t count) :
    m_size(count * static_cast<uint32_t>(sizeof(glm::mat4)))
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    std::array<VkDescriptorPoolSize, 1> poolSizes{ poolSize };
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.maxSets = 1;

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor pool" << std::endl;
        std::terminate();
    }

    VkDescriptorSetLayoutBinding transformLayoutBinding{};
    transformLayoutBinding.binding = 0;
    transformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    transformLayoutBinding.descriptorCount = 1;
    transformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo modelDescSetLayoutInfo{};
    modelDescSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayoutBinding, 1> modelBindings = { transformLayoutBinding };
    modelDescSetLayoutInfo.bindingCount = static_cast<uint32_t>(modelBindings.size());
    modelDescSetLayoutInfo.pBindings = modelBindings.data();

    VkResult result = vkCreateDescriptorSetLayout(device, &modelDescSetLayoutInfo, nullptr, &m_descSetLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create model descriptor set layout!" << std::endl;
        std::terminate();
    }

    VkDescriptorSetAllocateInfo descSetAllocInfo{};
    descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetAllocInfo.descriptorPool = m_descriptorPool;
    descSetAllocInfo.descriptorSetCount = 1;
    descSetAllocInfo.pSetLayouts = &m_descSetLayout;

    VkResult result = vkAllocateDescriptorSets(device, &descSetAllocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        std::cout << "Failed to allocate the descriptor set for Transforms" << std::endl;
        std::terminate();
    }

    m_uniformBuffer = std::make_unique<Buffer>(physicalDevice, device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Renderer::BUFFER_COUNT * m_size);
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = m_uniformBuffer->m_vkBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = Renderer::BUFFER_COUNT * m_size;

    VkWriteDescriptorSet uniformBufferDescriptorWrite{};
    uniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformBufferDescriptorWrite.dstSet = m_descriptorSet;
    uniformBufferDescriptorWrite.dstBinding = 0;
    uniformBufferDescriptorWrite.dstArrayElement = 0;
    uniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uniformBufferDescriptorWrite.descriptorCount = 1;
    uniformBufferDescriptorWrite.pBufferInfo = &uniformBufferInfo;

    vkUpdateDescriptorSets(device, 1, &uniformBufferDescriptorWrite, 0, nullptr);
}

void Transforms::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, uint32_t index)
{
    const uint32_t frameOffset = bufferIdx * m_size;
    const uint32_t totalOffset = frameOffset + index * sizeof(glm::mat4);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &m_descriptorSet, 1, &totalOffset);
}

void Transforms::update(uint32_t index, glm::mat4 transform, uint32_t bufferIdx)
{
    const uint32_t frameOffset = bufferIdx * m_size;
    const uint32_t totalOffset = frameOffset + index * sizeof(glm::mat4);

    m_uniformBuffer->update(&transform, totalOffset, sizeof(glm::mat4));
}