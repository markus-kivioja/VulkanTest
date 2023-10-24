#include "Tranforms.h"

#include <iostream>

Transforms::Transforms(uint32_t count) :
    m_size(count * static_cast<uint32_t>(sizeof(glm::mat4)))
{
    VkResult result = vkAllocateDescriptorSets(device, &m_descSetAllocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        std::cout << "Failed to allocate the descriptor set for Transforms" << std::endl;
        std::terminate();
    }

    m_uniformBuffer = std::make_unique<Buffer>(m_physicalDevice, m_device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Renderer::BUFFER_COUNT * m_size);
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = m_uniformBuffers[i]->m_vkBuffer;
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

    vkUpdateDescriptorSets(m_device, 1, &uniformBufferDescriptorWrite, 0, nullptr);
}

void Transforms::bind(VkCommandBuffer commandBuffer, uint32_t index, uint32_t bufferIdx)
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